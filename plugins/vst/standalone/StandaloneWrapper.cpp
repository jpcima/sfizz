// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "StandaloneWrapper.h"
#include "../SfizzVstIDs.h"
#include <cstdio>
#include <cassert>

static const FUID kProcessorComponentID = SfizzVstProcessor_cid;
static const FUID kControllerComponentID = SfizzVstController_cid;

StandaloneWrapper::StandaloneWrapper()
    : BaseWrapper(createConfig())
{
}

void StandaloneWrapper::setupProcessTimeInfo()
{
    
}

bool StandaloneWrapper::_sizeWindow(int32 width, int32 height)
{
    
    return false;
}

tresult PLUGIN_API StandaloneWrapper::getName(Vst::String128 name)
{
    const String theName { "Standalone" };
    theName.copyTo(name);
    return kResultTrue;
}

tresult PLUGIN_API StandaloneWrapper::beginEdit(Vst::ParamID id)
{
    
    return kResultFalse;
}

tresult PLUGIN_API StandaloneWrapper::performEdit(Vst::ParamID id, Vst::ParamValue valueNormalized)
{
    
    return kResultFalse;
}

tresult PLUGIN_API StandaloneWrapper::endEdit(Vst::ParamID id)
{
    
    return kResultFalse;
}

auto StandaloneWrapper::createConfig() -> SVST3Config&
{
    SVST3Config& config = config_;

    IPluginFactory* factory = GetPluginFactory();
    config.factory = factory;

    Vst::IAudioProcessor* processor = nullptr;
    factory->createInstance(kProcessorComponentID.toTUID(), Vst::IAudioProcessor::iid, reinterpret_cast<void**>(&processor));
    assert(processor);
    config.processor = processor;

    Vst::IEditController* controller = nullptr;
    factory->createInstance(kControllerComponentID.toTUID(), Vst::IEditController::iid, reinterpret_cast<void**>(&controller));
    assert(controller);
    config.controller = controller;

    return config;
}
