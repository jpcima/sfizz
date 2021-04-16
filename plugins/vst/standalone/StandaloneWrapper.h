// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "public.sdk/source/vst/basewrapper/basewrapper.h"

using namespace Steinberg;

class StandaloneWrapper : public Vst::BaseWrapper {
protected:
    StandaloneWrapper();
    virtual ~StandaloneWrapper() {}

public:
    Vst::IAudioProcessor* getProcessor() const { return mProcessor; }
    Vst::IEditController* getController() const { return mController; }

protected:
    void setupProcessTimeInfo() override;

    bool _sizeWindow(int32 width, int32 height) override;

    tresult PLUGIN_API getName(Vst::String128 name) override;

    tresult PLUGIN_API beginEdit(Vst::ParamID id) override;
    tresult PLUGIN_API performEdit(Vst::ParamID id, Vst::ParamValue valueNormalized) override;
    tresult PLUGIN_API endEdit(Vst::ParamID id) override;

private:
    SVST3Config& createConfig();

protected:
    SVST3Config config_;
};
