// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstState.h"
#include "sfizz/RTSemaphore.h"
#include "ring_buffer/ring_buffer.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include <sfizz.hpp>
#include <thread>
#include <mutex>
#include <memory>
#include <cstdlib>

using namespace Steinberg;

class SfizzVstProcessor : public Vst::AudioEffect {
public:
    SfizzVstProcessor();
    ~SfizzVstProcessor();

    tresult PLUGIN_API initialize(FUnknown* context) override;
    tresult PLUGIN_API setBusArrangements(Vst::SpeakerArrangement* inputs, int32 numIns, Vst::SpeakerArrangement* outputs, int32 numOuts) override;

    tresult PLUGIN_API setState(IBStream* stream) override;
    tresult PLUGIN_API getState(IBStream* stream) override;
    void syncStateToSynth();

    tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;
    tresult PLUGIN_API setActive(TBool state) override;
    tresult PLUGIN_API process(Vst::ProcessData& data) override;
    void processParameterChanges(Vst::IParameterChanges& pc);
    void processControllerChanges(Vst::IParameterChanges& pc);
    void processEvents(Vst::IEventList& events);
    static int convertVelocityFromFloat(float x);

    tresult PLUGIN_API notify(Vst::IMessage* message) override;

    static FUnknown* createInstance(void*);

    static FUID cid;

    // --- Sfizz stuff here below ---
private:
    // synth state. acquire processMutex before accessing
    std::unique_ptr<sfz::Sfizz> _synth;
    SfizzVstState _state;
    float _currentStretchedTuning = 0;

    // misc
    static void loadSfzFileOrDefault(sfz::Sfizz& synth, const std::string& filePath);

    // worker and thread sync
    std::thread _worker;
    volatile bool _workRunning = false;
    Ring_Buffer _fifoToWorker;
    RTSemaphore _semaToWorker;
    std::mutex _processMutex;

    // file modification periodic checker
    uint32 _fileChangeCounter = 0;
    uint32 _fileChangePeriod = 0;

    // state notification periodic timer
    uint32 _playStateChangeCounter = 0;
    uint32 _playStateChangePeriod = 0;

    // time info
    int _timeSigNumerator = 0;
    int _timeSigDenominator = 0;
    void updateTimeInfo(const Vst::ProcessContext& context);

    // controller state
    bool _haveCCNotification = false;
    int _ccnNumber = 0;
    float _ccnValue = 0.0f;

    // messaging
    struct RTMessage {
        const char* type;
        uintptr_t size;
        // 32-bit aligned data after header
        template <class T> const T* payload() const;
    };
    struct RTMessageDelete {
        void operator()(RTMessage* x) const noexcept { std::free(x); }
    };
    typedef std::unique_ptr<RTMessage, RTMessageDelete> RTMessagePtr;

    // worker
    void doBackgroundWork();
    void stopBackgroundWork();
    // writer
    bool writeWorkerMessage(const char* type, const void* data, uintptr_t size);
    // reader
    RTMessagePtr readWorkerMessage();
    bool discardWorkerMessage();
};

//------------------------------------------------------------------------------

template <class T> const T* SfizzVstProcessor::RTMessage::payload() const
{
    return reinterpret_cast<const T*>(
        reinterpret_cast<const uint8*>(this) + sizeof(*this));
}
