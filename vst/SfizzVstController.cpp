// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstController.h"
#include "SfizzVstEditor.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

tresult PLUGIN_API SfizzVstControllerNoUi::initialize(FUnknown* context)
{
    tresult result = EditController::initialize(context);
    if (result != kResultTrue)
        return result;

    Vst::ParamID pid = 0;

    // Ordinary parameters
    parameters.addParameter(
        kParamVolumeRange.createParameter(
            Steinberg::String("Volume"), pid++, Steinberg::String("dB"),
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamNumVoicesRange.createParameter(
            Steinberg::String("Polyphony"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamOversamplingRange.createParameter(
            Steinberg::String("Oversampling"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamPreloadSizeRange.createParameter(
            Steinberg::String("Preload size"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamScalaRootKeyRange.createParameter(
            Steinberg::String("Scala root key"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamTuningFrequencyRange.createParameter(
            Steinberg::String("Tuning frequency"), pid++, Steinberg::String("Hz"),
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));
    parameters.addParameter(
        kParamStretchedTuningRange.createParameter(
            Steinberg::String("Stretched tuning"), pid++, nullptr,
            0, Vst::ParameterInfo::kCanAutomate, Vst::kRootUnitId));

    // MIDI special controllers
    parameters.addParameter(Steinberg::String("Aftertouch"), nullptr, 0, 0.5, 0, pid++, Vst::kRootUnitId);
    parameters.addParameter(Steinberg::String("Pitch bend"), nullptr, 0, 0.5, 0, pid++, Vst::kRootUnitId);

    // MIDI controllers
    for (unsigned i = 0; i < kNumControllerParams; ++i) {
        Steinberg::String title;
        Steinberg::String shortTitle;
        title.printf("Controller %u", i);
        shortTitle.printf("CC%u", i);

        parameters.addParameter(
            title, nullptr, 0, 0, Vst::ParameterInfo::kNoFlags,
            pid++, Vst::kRootUnitId, shortTitle);
    }

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstControllerNoUi::terminate()
{
    return EditController::terminate();
}

tresult PLUGIN_API SfizzVstControllerNoUi::getMidiControllerAssignment(int32 busIndex, int16 channel, Vst::CtrlNumber midiControllerNumber, Vst::ParamID& id)
{
    switch (midiControllerNumber) {
    case Vst::kAfterTouch:
        id = kPidMidiAftertouch;
        return kResultTrue;

    case Vst::kPitchBend:
        id = kPidMidiPitchBend;
        return kResultTrue;

    default:
        if (midiControllerNumber < 0 || midiControllerNumber >= kNumControllerParams)
            return kResultFalse;

        id = kPidMidiCC0 + midiControllerNumber;
        return kResultTrue;
    }
}

tresult PLUGIN_API SfizzVstControllerNoUi::getParamStringByValue(Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
    switch (tag) {
    case kPidOversampling:
        {
            auto factorLog2 = static_cast<int>(kParamOversamplingRange.denormalize(valueNormalized));
            Steinberg::String buf;
            buf.printf("%dX", 1 << factorLog2);
            buf.copyTo(string);
            return kResultTrue;
        }
    }

    return EditController::getParamStringByValue(tag, valueNormalized, string);
}

tresult PLUGIN_API SfizzVstControllerNoUi::getParamValueByString(Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
    switch (tag) {
    case kPidOversampling:
        {
            int32 factor;
            if (!Steinberg::String::scanInt32(string, factor, false) || factor < 1)
                factor = 1;

            int32 log2Factor = 0;
            for (int32 f = factor; f > 1; f /= 2)
                ++log2Factor;

            valueNormalized = kParamOversamplingRange.normalize(log2Factor);
            return kResultTrue;
        }
    }

    return EditController::getParamValueByString(tag, string, valueNormalized);
}

// --- Controller with UI --- //

IPlugView* PLUGIN_API SfizzVstController::createView(FIDString _name)
{
    ConstString name(_name);

    fprintf(stderr, "[sfizz] about to create view: %s\n", _name);

    if (name != Vst::ViewType::kEditor)
        return nullptr;

    return new SfizzVstEditor(this);
}

tresult PLUGIN_API SfizzVstController::setParamNormalized(Vst::ParamID tag, Vst::ParamValue normValue)
{
    tresult r = SfizzVstControllerNoUi::setParamNormalized(tag, normValue);
    if (r != kResultTrue)
        return r;

    float *slotF32 = nullptr;
    int32 *slotI32 = nullptr;
    float value = 0;

    switch (tag) {
    case kPidVolume: {
        slotF32 = &_state.volume;
        value = kParamVolumeRange.denormalize(normValue);
        break;
    }
    case kPidNumVoices: {
        slotI32 = &_state.numVoices;
        value = kParamNumVoicesRange.denormalize(normValue);
        break;
    }
    case kPidOversampling: {
        slotI32 = &_state.oversamplingLog2;
        value = kParamOversamplingRange.denormalize(normValue);
        break;
    }
    case kPidPreloadSize: {
        slotI32 = &_state.preloadSize;
        value = kParamPreloadSizeRange.denormalize(normValue);
        break;
    }
    case kPidScalaRootKey: {
        slotI32 = &_state.scalaRootKey;
        value = kParamScalaRootKeyRange.denormalize(normValue);
        break;
    }
    case kPidTuningFrequency: {
        slotF32 = &_state.tuningFrequency;
        value = kParamTuningFrequencyRange.denormalize(normValue);
        break;
    }
    case kPidStretchedTuning: {
        slotF32 = &_state.stretchedTuning;
        value = kParamStretchedTuningRange.denormalize(normValue);
        break;
    }
    }

    bool update = false;

    if (slotF32 && *slotF32 != value) {
        *slotF32 = value;
        update = true;
    }
    else if (slotI32 && *slotI32 != (int32)value) {
        *slotI32 = (int32)value;
        update = true;
    }

    if (update) {
        for (StateListener* listener : _stateListeners)
            listener->onStateChanged();
    }

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstController::setState(IBStream* state)
{
    SfizzUiState s;

    tresult r = s.load(state);
    if (r != kResultTrue)
        return r;

    _uiState = s;

    for (StateListener* listener : _stateListeners)
        listener->onStateChanged();

    return kResultTrue;
}

tresult PLUGIN_API SfizzVstController::getState(IBStream* state)
{
    return _uiState.store(state);
}

tresult PLUGIN_API SfizzVstController::setComponentState(IBStream* state)
{
    SfizzVstState s;

    tresult r = s.load(state);
    if (r != kResultTrue)
        return r;

    _state = s;

    setParamNormalized(kPidVolume, kParamVolumeRange.normalize(s.volume));
    setParamNormalized(kPidNumVoices, kParamNumVoicesRange.normalize(s.numVoices));
    setParamNormalized(kPidOversampling, kParamOversamplingRange.normalize(s.oversamplingLog2));
    setParamNormalized(kPidPreloadSize, kParamPreloadSizeRange.normalize(s.preloadSize));
    setParamNormalized(kPidScalaRootKey, kParamScalaRootKeyRange.normalize(s.scalaRootKey));
    setParamNormalized(kPidTuningFrequency, kParamTuningFrequencyRange.normalize(s.tuningFrequency));
    setParamNormalized(kPidStretchedTuning, kParamStretchedTuningRange.normalize(s.stretchedTuning));

    for (StateListener* listener : _stateListeners)
        listener->onStateChanged();

    return kResultTrue;
}

tresult SfizzVstController::notify(Vst::IMessage* message)
{
    tresult result = SfizzVstControllerNoUi::notify(message);
    if (result != kResultFalse)
        return result;

    const char* id = message->getMessageID();
    Vst::IAttributeList* attr = message->getAttributes();

    if (!strcmp(id, "LoadedSfz")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("File", data, size);

        if (result != kResultTrue)
            return result;

        _state.sfzFile.assign(static_cast<const char *>(data), size);
    }
    else if (!strcmp(id, "LoadedScala")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("File", data, size);

        if (result != kResultTrue)
            return result;

        _state.scalaFile.assign(static_cast<const char *>(data), size);
    }
    else if (!strcmp(id, "NotifiedPlayState")) {
        const void* data = nullptr;
        uint32 size = 0;
        result = attr->getBinary("PlayState", data, size);

        if (result != kResultTrue)
            return result;

        _playState = *static_cast<const SfizzPlayState*>(data);
    }
    else if (!strcmp(id, "NotifiedController")) {
        int64 ccNumber;
        double ccValue;

        result = attr->getInt("Number", ccNumber);
        if (result != kResultTrue)
            return result;

        result = attr->getFloat("Value", ccValue);
        if (result != kResultTrue)
            return result;

        for (ControllerChangeListener* listener : _ccListeners)
            listener->onControllerChange(ccNumber, ccValue);
    }

    for (StateListener* listener : _stateListeners)
        listener->onStateChanged();

    return result;
}

void SfizzVstController::addSfizzStateListener(StateListener* listener)
{
    _stateListeners.push_back(listener);
}

void SfizzVstController::removeSfizzStateListener(StateListener* listener)
{
    auto it = std::find(_stateListeners.begin(), _stateListeners.end(), listener);
    if (it != _stateListeners.end())
        _stateListeners.erase(it);
}

void SfizzVstController::addSfizzControllerChangeListener(ControllerChangeListener* listener)
{
    _ccListeners.push_back(listener);
}

void SfizzVstController::removeSfizzControllerChangeListener(ControllerChangeListener* listener)
{
    auto it = std::find(_ccListeners.begin(), _ccListeners.end(), listener);
    if (it != _ccListeners.end())
        _ccListeners.erase(it);
}

FUnknown* SfizzVstController::createInstance(void*)
{
    return static_cast<Vst::IEditController*>(new SfizzVstController);
}

/*
  Note(jpc) Generated at random with uuidgen.
  Can't find docs on it... maybe it's to register somewhere?
 */
FUID SfizzVstController::cid(0x7129736c, 0xbc784134, 0xbb899d56, 0x2ebafe4f);
