// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstState.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"
class SfizzVstState;

using namespace Steinberg;
using namespace VSTGUI;

class SfizzVstControllerNoUi : public Vst::EditController,
                               public Vst::IMidiMapping {
public:
    virtual ~SfizzVstControllerNoUi() {}

    tresult PLUGIN_API initialize(FUnknown* context) override;
    tresult PLUGIN_API terminate() override;

    tresult PLUGIN_API getMidiControllerAssignment(int32 busIndex, int16 channel, Vst::CtrlNumber midiControllerNumber, Vst::ParamID& id) override;

    tresult PLUGIN_API getParamStringByValue(Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string) override;
    tresult PLUGIN_API getParamValueByString(Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized) override;


    // interfaces
    OBJ_METHODS(SfizzVstControllerNoUi, Vst::EditController)
    DEFINE_INTERFACES
    DEF_INTERFACE(Vst::IMidiMapping)
    END_DEFINE_INTERFACES(Vst::EditController)
    REFCOUNT_METHODS(Vst::EditController)
};

class SfizzVstController : public SfizzVstControllerNoUi, public VSTGUI::VST3EditorDelegate {
public:
    IPlugView* PLUGIN_API createView(FIDString name) override;

    tresult PLUGIN_API setParamNormalized(Vst::ParamID tag, Vst::ParamValue value) override;
    tresult PLUGIN_API setState(IBStream* state) override;
    tresult PLUGIN_API getState(IBStream* state) override;
    tresult PLUGIN_API setComponentState(IBStream* state) override;
    tresult PLUGIN_API notify(Vst::IMessage* message) override;

    struct StateListener {
        virtual void onStateChanged() = 0;
    };
    struct ControllerChangeListener {
        virtual void onControllerChange(int ccNumber, float ccValue) = 0;
    };

    const SfizzVstState& getSfizzState() const { return _state; }
    SfizzVstState& getSfizzState() { return _state; }

    const SfizzUiState& getSfizzUiState() const { return _uiState; }
    SfizzUiState& getSfizzUiState() { return _uiState; }

    const SfizzPlayState& getSfizzPlayState() const { return _playState; }
    SfizzPlayState& getSfizzPlayState() { return _playState; }

    void addSfizzStateListener(StateListener* listener);
    void removeSfizzStateListener(StateListener* listener);

    void addSfizzControllerChangeListener(ControllerChangeListener* listener);
    void removeSfizzControllerChangeListener(ControllerChangeListener* listener);

    ///
    static FUnknown* createInstance(void*);

    static FUID cid;

private:
    SfizzVstState _state;
    SfizzUiState _uiState;
    SfizzPlayState _playState {};
    std::vector<StateListener*> _stateListeners;
    std::vector<ControllerChangeListener*> _ccListeners;
};
