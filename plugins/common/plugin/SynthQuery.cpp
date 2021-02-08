// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SynthQuery.h"
#include "MessageUtils.h"
#include <cstdio>

BitArray<sfz::config::numCCs> queryCCSlots(sfz::Sfizz& synth, int delay)
{
    struct ClientData {
        BitArray<sfz::config::numCCs> slots;
    };

    auto receive = [](void* data, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
    {
        ClientData& cdata = *reinterpret_cast<ClientData*>(data);
        unsigned indices[0];
        (void)delay;
        if (Messages::matchOSC("/cc/slots", path, indices) && !strcmp(sig, "b")) {
            sfizz_blob_t b = *args[0].b;
            uint32_t n = std::min(b.size, static_cast<uint32_t>(cdata.slots.byte_size()));
            std::memcpy(cdata.slots.data(), b.data, n);
        }
    };

    ClientData cdata;

    sfz::ClientPtr client = sfz::Sfizz::createClient(&cdata);
    sfz::Sfizz::setReceiveCallback(*client, receive);

    const char* path = "/cc/slots";
    synth.sendMessage(*client, delay, path, "", nullptr);

    return cdata.slots;
}

float queryCCDefaultValue(sfz::Sfizz& synth, uint32_t cc, int delay)
{
    struct ClientData {
        uint32_t cc;
        float value;
    };

    auto receive = [](void* data, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
    {
        ClientData& cdata = *reinterpret_cast<ClientData*>(data);
        unsigned indices[1];
        (void)delay;
        if (Messages::matchOSC("/cc&/default", path, indices) && !strcmp(sig, "f") && indices[0] == cdata.cc)
            cdata.value = args[0].f;
    };

    ClientData cdata;
    cdata.cc = cc;

    sfz::ClientPtr client = sfz::Sfizz::createClient(&cdata);
    sfz::Sfizz::setReceiveCallback(*client, receive);

    char path[256];
    std::sprintf(path, "/cc%u/default", cc);
    synth.sendMessage(*client, delay, path, "", nullptr);

    return cdata.value;
}

std::string queryCCLabel(sfz::Sfizz& synth, uint32_t cc, int delay)
{
    struct ClientData {
        uint32_t cc;
        std::string label;
    };

    auto receive = [](void* data, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
    {
        ClientData& cdata = *reinterpret_cast<ClientData*>(data);
        unsigned indices[1];
        (void)delay;
        if (Messages::matchOSC("/cc&/label", path, indices) && !strcmp(sig, "s") && indices[0] == cdata.cc)
            cdata.label.assign(args[0].s);
    };

    ClientData cdata;
    cdata.cc = cc;

    sfz::ClientPtr client = sfz::Sfizz::createClient(&cdata);
    sfz::Sfizz::setReceiveCallback(*client, receive);

    char path[256];
    std::sprintf(path, "/cc%u/label", cc);
    synth.sendMessage(*client, delay, path, "", nullptr);

    return cdata.label;
}
