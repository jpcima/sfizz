// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "JACKWrapper.h"
#include "VstPluginDefs.h"
#include <stdexcept>
#include <cstdio>

JACKWrapper::JACKWrapper()
{
}

JACKWrapper::~JACKWrapper()
{
    jack_client_t* client = client_;
    if (client) {
        if (jack_port_t* port = midiPort_)
            jack_port_unregister(client, port);
        for (jack_port_t* port : inputPorts_)
            jack_port_unregister(client, port);
        for (jack_port_t* port : outputPorts_)
            jack_port_unregister(client, port);
        jack_client_close(client);
    }
}

bool JACKWrapper::init()
{
    jack_client_t* client = client_;
    if (!client) {
        client = jack_client_open(VSTPLUGIN_NAME, JackNoStartServer, nullptr);
        if (!client)
            return false;
        client_ = client;
        jack_set_process_callback(client, &JACKprocess, this);
    }

    mSampleRate = jack_get_sample_rate(client);
    mBlockSize = jack_get_buffer_size(client);

    jack_port_t* midiPort = midiPort_;
    if (!midiPort) {
        midiPort = jack_port_register(client, "Events", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
        if (!midiPort)
            return false;
        midiPort_ = midiPort;
    }

    ///
    if (!StandaloneWrapper::init())
        return false;

    ///
    return true;
}

void JACKWrapper::activate()
{
    _resume();
    if (jack_activate(client_) != 0)
        throw std::runtime_error("Could not activate JACK client");
}

void JACKWrapper::deactivate()
{
    if (jack_deactivate(client_) != 0)
        throw std::runtime_error("Could not deactivate JACK client");
    _suspend();
}

void JACKWrapper::setupBuses()
{
    StandaloneWrapper::setupBuses();

    const int32 numInputs = mNumInputs;
    const int32 numOutputs = mNumOutputs;

    ///
    jack_client_t* client = client_;

    for (jack_port_t* port : inputPorts_)
        jack_port_unregister(client, port);
    inputPorts_.clear();

    for (jack_port_t* port : outputPorts_)
        jack_port_unregister(client, port);
    outputPorts_.clear();

    ///
    for (int32 i = 0; i < numInputs; ++i) {
        String name;
        name.printf("Input-%d", i + 1);
        jack_port_t* port = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        if (!port)
            throw std::runtime_error("Could not create JACK input");
        inputPorts_.push_back(port);
    }
    inputChannels_.resize(numInputs);

    for (int32 i = 0; i < numOutputs; ++i) {
        String name;
        name.printf("Output-%d", i + 1);
        jack_port_t* port = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        if (!port)
            throw std::runtime_error("Could not create JACK output");
        outputPorts_.push_back(port);
    }
    outputChannels_.resize(numOutputs);
}

int JACKWrapper::JACKprocess(jack_nframes_t nframes, void* arg)
{
    JACKWrapper* self = reinterpret_cast<JACKWrapper*>(arg);

    ///
    void* midiBuffer = jack_port_get_buffer(self->midiPort_, nframes);
    uint32 midiEventCount = jack_midi_get_event_count(midiBuffer);
    for (uint32 i = 0; i < midiEventCount; ++i) {
        jack_midi_event_t event;
        if (jack_midi_event_get(&event, midiBuffer, i) == 0) {
            Vst::Event vstEvent {};
            vstEvent.sampleOffset = static_cast<int32>(event.time);
            self->processMidiEvent(vstEvent, reinterpret_cast<char*>(event.buffer));
        }
    }

    ///
    const int32 numInputs = self->mNumInputs;
    const int32 numOutputs = self->mNumOutputs;
    float** inputChannels = self->inputChannels_.data();
    float** outputChannels = self->outputChannels_.data();
    jack_port_t** inputPorts = self->inputPorts_.data();
    jack_port_t** outputPorts = self->outputPorts_.data();

    for (int32 i = 0; i < numInputs; ++i)
        inputChannels[i] = reinterpret_cast<float*>(jack_port_get_buffer(inputPorts[i], nframes));
    for (int32 i = 0; i < numOutputs; ++i)
        outputChannels[i] = reinterpret_cast<float*>(jack_port_get_buffer(outputPorts[i], nframes));

    self->_processReplacing(inputChannels, outputChannels, nframes);

    return 0;
}
