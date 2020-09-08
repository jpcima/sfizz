// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstProcessor.h"
#include "SfizzVstController.h"
#include "SfizzVstState.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <cstring>

template<class T>
constexpr int fastRound(T x)
{
    return static_cast<int>(x + T{ 0.5 }); // NOLINT
}

static const char defaultSfzText[] =
    "<region>sample=*sine" "\n"
    "ampeg_attack=0.02 ampeg_release=0.1" "\n";

SfizzVstProcessor::SfizzVstProcessor()
    : _fifoToWorker(64 * 1024)
{
    setControllerClass(SfizzVstController::cid);
}

SfizzVstProcessor::~SfizzVstProcessor()
{
    try {
        stopBackgroundWork();
    } catch (const std::exception& e) {
        fprintf(stderr, "Caught exception: %s\n", e.what());
    }
}

tresult PLUGIN_API SfizzVstProcessor::initialize(FUnknown* context)
{
    tresult result = AudioEffect::initialize(context);
    if (result != kResultTrue)
        return result;

    addAudioOutput(STR16("Audio Output"), Vst::SpeakerArr::kStereo);
    addEventInput(STR16("Event Input"), 1);

    _state = SfizzVstState();

    fprintf(stderr, "[sfizz] new synth\n");
    _synth.reset(new sfz::Sfizz);
    _currentStretchedTuning = 0.0;
    loadSfzFileOrDefault(*_synth, {});

    _synth->tempo(0, 0.5);
    _timeSigNumerator = 4;
    _timeSigDenominator = 4;
    _synth->timeSignature(0, _timeSigNumerator, _timeSigDenominator);
    _synth->timePosition(0, 0, 0);
    _synth->playbackState(0, 0);

    _haveCCNotification = true;
    _ccnNumber = -1;
    _ccnValue = 0.0f;

    return result;
}

tresult PLUGIN_API SfizzVstProcessor::setBusArrangements(Vst::SpeakerArrangement* inputs, int32 numIns, Vst::SpeakerArrangement* outputs, int32 numOuts)
{
    bool isStereo = numIns == 0 && numOuts == 1 && outputs[0] == Vst::SpeakerArr::kStereo;

    if (!isStereo)
        return kResultFalse;

    return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
}

tresult PLUGIN_API SfizzVstProcessor::setState(IBStream* stream)
{
    SfizzVstState s;

    tresult r = s.load(stream);
    if (r != kResultTrue)
        return r;

    std::lock_guard<std::mutex> lock(_processMutex);
    _state = s;

    syncStateToSynth();

    return r;
}

tresult PLUGIN_API SfizzVstProcessor::getState(IBStream* stream)
{
    std::lock_guard<std::mutex> lock(_processMutex);
    return _state.store(stream);
}

void SfizzVstProcessor::syncStateToSynth()
{
    sfz::Sfizz* synth = _synth.get();

    if (!synth)
        return;

    loadSfzFileOrDefault(*synth, _state.sfzFile);
    synth->setVolume(_state.volume);
    synth->setNumVoices(_state.numVoices);
    synth->setOversamplingFactor(1 << _state.oversamplingLog2);
    synth->setPreloadSize(_state.preloadSize);
    synth->loadScalaFile(_state.scalaFile);
    synth->setScalaRootKey(_state.scalaRootKey);
    synth->setTuningFrequency(_state.tuningFrequency);
    synth->loadStretchTuningByRatio(_state.stretchedTuning);
}

tresult PLUGIN_API SfizzVstProcessor::canProcessSampleSize(int32 symbolicSampleSize)
{
    if (symbolicSampleSize != Vst::kSample32)
        return kResultFalse;

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstProcessor::setActive(TBool state)
{
    sfz::Sfizz* synth = _synth.get();

    if (!synth)
        return kResultFalse;

    if (state) {
        synth->setSampleRate(processSetup.sampleRate);
        synth->setSamplesPerBlock(processSetup.maxSamplesPerBlock);

        _fileChangePeriod = static_cast<uint32>(1.0 * processSetup.sampleRate);
        _playStateChangePeriod = static_cast<uint32>(50e-3 * processSetup.sampleRate);

        _workRunning = true;
        _worker = std::thread([this]() { doBackgroundWork(); });
    } else {
        synth->allSoundOff();
        stopBackgroundWork();
    }

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstProcessor::process(Vst::ProcessData& data)
{
    sfz::Sfizz& synth = *_synth;

    if (data.processContext)
        updateTimeInfo(*data.processContext);

    if (Vst::IParameterChanges* pc = data.inputParameterChanges)
        processParameterChanges(*pc);

    if (data.numOutputs < 1)  // flush mode
        return kResultTrue;

    uint32 numFrames = data.numSamples;
    constexpr uint32 numChannels = 2;
    float* outputs[numChannels];

    assert(numChannels == data.outputs[0].numChannels);

    for (unsigned c = 0; c < numChannels; ++c)
        outputs[c] = data.outputs[0].channelBuffers32[c];

    std::unique_lock<std::mutex> lock(_processMutex, std::try_to_lock);

    if (!lock.owns_lock()) {
        for (unsigned c = 0; c < numChannels; ++c)
            std::memset(outputs[c], 0, numFrames * sizeof(float));
        data.outputs[0].silenceFlags = 3;
        return kResultTrue;
    }

    if (data.processMode == Vst::kOffline)
        synth.enableFreeWheeling();
    else
        synth.disableFreeWheeling();

    if (Vst::IParameterChanges* pc = data.inputParameterChanges)
        processControllerChanges(*pc);

    if (Vst::IEventList* events = data.inputEvents)
        processEvents(*events);

    synth.setVolume(_state.volume);
    synth.setScalaRootKey(_state.scalaRootKey);
    synth.setTuningFrequency(_state.tuningFrequency);
    if (_currentStretchedTuning != _state.stretchedTuning) {
        synth.loadStretchTuningByRatio(_state.stretchedTuning);
        _currentStretchedTuning = _state.stretchedTuning;
    }

    synth.renderBlock(outputs, numFrames, numChannels);

    _fileChangeCounter += numFrames;
    if (_fileChangeCounter > _fileChangePeriod) {
        _fileChangeCounter %= _fileChangePeriod;
        if (writeWorkerMessage("CheckShouldReload", nullptr, 0))
            _semaToWorker.post();
    }

    _playStateChangeCounter += numFrames;
    if (_playStateChangeCounter > _playStateChangePeriod) {
        _playStateChangeCounter %= _playStateChangePeriod;
        SfizzPlayState playState;
        playState.curves = synth.getNumCurves();
        playState.masters = synth.getNumMasters();
        playState.groups = synth.getNumGroups();
        playState.regions = synth.getNumRegions();
        playState.preloadedSamples = synth.getNumPreloadedSamples();
        playState.activeVoices = synth.getNumActiveVoices();
        if (writeWorkerMessage("NotifyPlayState", &playState, sizeof(playState)))
            _semaToWorker.post();
    }

    if (!_haveCCNotification)
        _haveCCNotification = synth.checkHdcc(_ccnNumber, _ccnValue);

    auto sendCCNotification = [this]() -> bool {
        if (!_haveCCNotification)
            return false;
        std::pair<int, float> payload { _ccnNumber, _ccnValue };
        if (!writeWorkerMessage("NotifyController", &payload, sizeof(payload)))
            return false;
        _semaToWorker.post();
        _haveCCNotification = false;
        return true;
    };

    while (sendCCNotification())
        _haveCCNotification = synth.checkHdcc(_ccnNumber, _ccnValue);

    return kResultTrue;
}

void SfizzVstProcessor::updateTimeInfo(const Vst::ProcessContext& context)
{
    sfz::Sfizz& synth = *_synth;

    if (context.state & context.kTempoValid)
        synth.tempo(0, 60.0f / context.tempo);

    if (context.state & context.kTimeSigValid) {
        _timeSigNumerator = context.timeSigNumerator;
        _timeSigDenominator = context.timeSigDenominator;
        synth.timeSignature(0, _timeSigNumerator, _timeSigDenominator);
    }

    if (context.state & context.kProjectTimeMusicValid) {
        double beats = context.projectTimeMusic * 0.25 * _timeSigDenominator;
        double bars = beats / _timeSigNumerator;
        beats -= int(bars) * _timeSigNumerator;
        synth.timePosition(0, int(bars), float(beats));
    }

    synth.playbackState(0, (context.state & context.kPlaying) != 0);
}

void SfizzVstProcessor::processParameterChanges(Vst::IParameterChanges& pc)
{
    uint32 paramCount = pc.getParameterCount();

    for (uint32 paramIndex = 0; paramIndex < paramCount; ++paramIndex) {
        Vst::IParamValueQueue* vq = pc.getParameterData(paramIndex);
        if (!vq)
            continue;

        Vst::ParamID id = vq->getParameterId();
        uint32 pointCount = vq->getPointCount();
        int32 sampleOffset;
        Vst::ParamValue value;

        switch (id) {
        case kPidVolume:
            if (pointCount > 0 && vq->getPoint(pointCount - 1, sampleOffset, value) == kResultTrue)
                _state.volume = kParamVolumeRange.denormalize(value);
            break;
        case kPidNumVoices:
            if (pointCount > 0 && vq->getPoint(pointCount - 1, sampleOffset, value) == kResultTrue) {
                int32 data = static_cast<int32>(kParamNumVoicesRange.denormalize(value));
                _state.numVoices = data;
                if (writeWorkerMessage("SetNumVoices", &data, sizeof(data)))
                    _semaToWorker.post();
            }
            break;
        case kPidOversampling:
            if (pointCount > 0 && vq->getPoint(pointCount - 1, sampleOffset, value) == kResultTrue) {
                int32 data = static_cast<int32>(kParamOversamplingRange.denormalize(value));
                _state.oversamplingLog2 = data;
                if (writeWorkerMessage("SetOversampling", &data, sizeof(data)))
                    _semaToWorker.post();
            }
            break;
        case kPidPreloadSize:
            if (pointCount > 0 && vq->getPoint(pointCount - 1, sampleOffset, value) == kResultTrue) {
                int32 data = static_cast<int32>(kParamPreloadSizeRange.denormalize(value));
                _state.preloadSize = data;
                if (writeWorkerMessage("SetPreloadSize", &data, sizeof(data)))
                    _semaToWorker.post();
            }
            break;
        case kPidScalaRootKey:
            if (pointCount > 0 && vq->getPoint(pointCount - 1, sampleOffset, value) == kResultTrue)
                _state.scalaRootKey = static_cast<int32>(kParamScalaRootKeyRange.denormalize(value));
            break;
        case kPidTuningFrequency:
            if (pointCount > 0 && vq->getPoint(pointCount - 1, sampleOffset, value) == kResultTrue)
                _state.tuningFrequency = kParamTuningFrequencyRange.denormalize(value);
            break;
        case kPidStretchedTuning:
            if (pointCount > 0 && vq->getPoint(pointCount - 1, sampleOffset, value) == kResultTrue)
                _state.stretchedTuning = kParamStretchedTuningRange.denormalize(value);
            break;
        }
    }
}

void SfizzVstProcessor::processControllerChanges(Vst::IParameterChanges& pc)
{
    sfz::Sfizz& synth = *_synth;
    uint32 paramCount = pc.getParameterCount();

    for (uint32 paramIndex = 0; paramIndex < paramCount; ++paramIndex) {
        Vst::IParamValueQueue* vq = pc.getParameterData(paramIndex);
        if (!vq)
            continue;

        Vst::ParamID id = vq->getParameterId();
        uint32 pointCount = vq->getPointCount();
        int32 sampleOffset;
        Vst::ParamValue value;

        switch (id) {
        default:
            if (id >= kPidMidiCC0 && id <= kPidMidiCCLast) {
                auto ccNumber = static_cast<int>(id - kPidMidiCC0);
                for (uint32 pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                    if (vq->getPoint(pointIndex, sampleOffset, value) == kResultTrue)
                        synth.cc(sampleOffset, ccNumber, fastRound(value * 127.0));
                }
            }
            break;

        case kPidMidiAftertouch:
            for (uint32 pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                if (vq->getPoint(pointIndex, sampleOffset, value) == kResultTrue)
                    synth.aftertouch(sampleOffset, fastRound(value * 127.0));
            }
            break;

        case kPidMidiPitchBend:
            for (uint32 pointIndex = 0; pointIndex < pointCount; ++pointIndex) {
                if (vq->getPoint(pointIndex, sampleOffset, value) == kResultTrue)
                    synth.pitchWheel(sampleOffset, fastRound(value * 16383) - 8192);
            }
            break;
        }
    }
}

void SfizzVstProcessor::processEvents(Vst::IEventList& events)
{
    sfz::Sfizz& synth = *_synth;
    uint32 numEvents = events.getEventCount();

    for (uint32 i = 0; i < numEvents; i++) {
        Vst::Event e;
        if (events.getEvent(i, e) != kResultTrue)
            continue;

        switch (e.type) {
        case Vst::Event::kNoteOnEvent:
            if (e.noteOn.velocity == 0.0f)
                synth.noteOff(e.sampleOffset, e.noteOn.pitch, 0);
            else
                synth.noteOn(e.sampleOffset, e.noteOn.pitch, convertVelocityFromFloat(e.noteOn.velocity));
            break;
        case Vst::Event::kNoteOffEvent:
            synth.noteOff(e.sampleOffset, e.noteOff.pitch, convertVelocityFromFloat(e.noteOff.velocity));
            break;
        // case Vst::Event::kPolyPressureEvent:
        //     synth.aftertouch(e.sampleOffset, convertVelocityFromFloat(e.polyPressure.pressure));
        //     break;
        }
    }
}

int SfizzVstProcessor::convertVelocityFromFloat(float x)
{
    return std::min(127, std::max(0, (int)(x * 127.0f)));
}

tresult PLUGIN_API SfizzVstProcessor::notify(Vst::IMessage* message)
{
    // Note(jpc) this notification is not necessarily handled by the RT thread

    tresult result = AudioEffect::notify(message);
    if (result != kResultFalse)
        return result;

    const char* id = message->getMessageID();
    Vst::IAttributeList* attr = message->getAttributes();

    if (!std::strcmp(id, "LoadSfz")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("File", data, size);

        if (result != kResultTrue)
            return result;

        std::unique_lock<std::mutex> lock(_processMutex);
        _state.sfzFile.assign(static_cast<const char *>(data), size);
        loadSfzFileOrDefault(*_synth, _state.sfzFile);
        lock.unlock();

        Steinberg::OPtr<Vst::IMessage> reply { allocateMessage() };
        reply->setMessageID("LoadedSfz");
        reply->getAttributes()->setBinary("File", _state.sfzFile.data(), _state.sfzFile.size());
        sendMessage(reply);
    }
    else if (!std::strcmp(id, "LoadScala")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("File", data, size);

        if (result != kResultTrue)
            return result;

        std::unique_lock<std::mutex> lock(_processMutex);
        _state.scalaFile.assign(static_cast<const char *>(data), size);
        _synth->loadScalaFile(_state.scalaFile);
        lock.unlock();

        Steinberg::OPtr<Vst::IMessage> reply { allocateMessage() };
        reply->setMessageID("LoadedScala");
        reply->getAttributes()->setBinary("File", _state.scalaFile.data(), _state.scalaFile.size());
        sendMessage(reply);
    }

    return result;
}

FUnknown* SfizzVstProcessor::createInstance(void*)
{
    return static_cast<Vst::IAudioProcessor*>(new SfizzVstProcessor);
}

void SfizzVstProcessor::loadSfzFileOrDefault(sfz::Sfizz& synth, const std::string& filePath)
{
    if (!filePath.empty())
        synth.loadSfzFile(filePath);
    else
        synth.loadSfzString("default.sfz", defaultSfzText);
}

void SfizzVstProcessor::doBackgroundWork()
{
    for (;;) {
        _semaToWorker.wait();

        if (!_workRunning)
            break;

        RTMessagePtr msg = readWorkerMessage();
        if (!msg) {
            fprintf(stderr, "[Sfizz] message synchronization error in worker\n");
            std::abort();
        }

        const char* id = msg->type;

        if (!std::strcmp(id, "SetNumVoices")) {
            int32 value = *msg->payload<int32>();
            _synth->setNumVoices(value);
        }
        else if (!std::strcmp(id, "SetOversampling")) {
            int32 value = *msg->payload<int32>();
            _synth->setOversamplingFactor(1 << value);
        }
        else if (!std::strcmp(id, "SetPreloadSize")) {
            int32 value = *msg->payload<int32>();
            _synth->setPreloadSize(value);
        }
        else if (!std::strcmp(id, "CheckShouldReload")) {
            if (_synth->shouldReloadFile()) {
                fprintf(stderr, "[Sfizz] sfz file has changed, reloading\n");
                loadSfzFileOrDefault(*_synth, _state.sfzFile);
            }
            else if (_synth->shouldReloadScala()) {
                fprintf(stderr, "[Sfizz] scala file has changed, reloading\n");
                _synth->loadScalaFile(_state.scalaFile);
            }
        }
        else if (!std::strcmp(id, "NotifyPlayState")) {
            SfizzPlayState playState = *msg->payload<SfizzPlayState>();
            Steinberg::OPtr<Vst::IMessage> notification { allocateMessage() };
            notification->setMessageID("NotifiedPlayState");
            notification->getAttributes()->setBinary("PlayState", &playState, sizeof(playState));
            sendMessage(notification);
        }
        else if (!std::strcmp(id, "NotifyController")) {
            std::pair<int, float> cc = *msg->payload<std::pair<int, float>>();
            Steinberg::OPtr<Vst::IMessage> notification { allocateMessage() };
            notification->setMessageID("NotifiedController");
            Vst::IAttributeList* attr = notification->getAttributes();
            attr->setInt("Number", cc.first);
            attr->setFloat("Value", cc.second);
            sendMessage(notification);
        }
    }
}

void SfizzVstProcessor::stopBackgroundWork()
{
    if (!_workRunning)
        return;

    _workRunning = false;
    _semaToWorker.post();
    _worker.join();

    while (_semaToWorker.try_wait()) {
        if (!discardWorkerMessage()) {
            fprintf(stderr, "[Sfizz] message synchronization error in processor\n");
            std::abort();
        }
    }
}

bool SfizzVstProcessor::writeWorkerMessage(const char* type, const void* data, uintptr_t size)
{
    RTMessage header;
    header.type = type;
    header.size = size;

    if (_fifoToWorker.size_free() < sizeof(header) + size)
        return false;

    _fifoToWorker.put(header);
    _fifoToWorker.put(static_cast<const uint8*>(data), size);
    return true;
}

SfizzVstProcessor::RTMessagePtr SfizzVstProcessor::readWorkerMessage()
{
    RTMessage header;

    if (!_fifoToWorker.peek(header))
        return nullptr;
    if (_fifoToWorker.size_used() < sizeof(header) + header.size)
        return nullptr;

    RTMessagePtr msg { reinterpret_cast<RTMessage*>(std::malloc(sizeof(header) + header.size)) };
    if (!msg)
        throw std::bad_alloc();

    msg->type = header.type;
    msg->size = header.size;
    _fifoToWorker.discard(sizeof(header));
    _fifoToWorker.get(const_cast<char*>(msg->payload<char>()), header.size);

    return msg;
}

bool SfizzVstProcessor::discardWorkerMessage()
{
    RTMessage header;

    if (!_fifoToWorker.peek(header))
        return false;
    if (_fifoToWorker.size_used() < sizeof(header) + header.size)
        return false;

    _fifoToWorker.discard(sizeof(header) + header.size);
    return true;
}

/*
  Note(jpc) Generated at random with uuidgen.
  Can't find docs on it... maybe it's to register somewhere?
 */
FUID SfizzVstProcessor::cid(0xe8fab718, 0x15ed46e3, 0x8b598310, 0x1e12993f);
