// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "StandaloneWrapper.h"
#include <jack/jack.h>
#include <jack/midiport.h>

class JACKWrapper final : public StandaloneWrapper {
public:
    JACKWrapper();
    ~JACKWrapper();

    bool init() override;

    void activate();
    void deactivate();

protected:
    void setupBuses() override;
    static int JACKprocess(jack_nframes_t nframes, void* arg);

private:
    jack_client_t* client_ = nullptr;
    jack_port_t* midiPort_ = nullptr;
    std::vector<jack_port_t*> inputPorts_;
    std::vector<jack_port_t*> outputPorts_;
    std::vector<float*> inputChannels_;
    std::vector<float*> outputChannels_;
};
