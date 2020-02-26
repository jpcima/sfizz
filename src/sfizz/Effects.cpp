// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Effects.h"
#include "Opcode.h"
#include "SIMDHelpers.h"
#include "Config.h"
#include "effects/Nothing.h"
#include "effects/Lofi.h"
#include <algorithm>

namespace sfz {

void EffectFactory::registerStandardEffectTypes()
{
    // TODO
    registerEffectType("lofi", fx::Lofi::makeInstance);
}

void EffectFactory::registerEffectType(absl::string_view name, Effect::MakeInstance& make)
{
    FactoryEntry ent;
    ent.name = std::string(name);
    ent.make = &make;
    _entries.push_back(std::move(ent));
}

Effect* EffectFactory::makeEffect(absl::Span<const Opcode> members)
{
    const Opcode* opcode = nullptr;

    for (auto it = members.rbegin(); it != members.rend() && !opcode; ++it) {
        if (it->lettersOnlyHash == hash("type"))
            opcode = &*it;
    }

    if (!opcode) {
        DBG("The effect does not specify a type");
        return new sfz::fx::Nothing;
    }

    absl::string_view type = opcode->value;

    auto it = _entries.begin();
    auto end = _entries.end();
    for (; it != end && it->name != type; ++it);

    if (it == end) {
        DBG("Unsupported effect type: " << type);
        return new sfz::fx::Nothing;
    }

    Effect* fx = it->make(members);
    if (!fx) {
        DBG("Could not instantiate effect of type: " << type);
        return new sfz::fx::Nothing;
    }

    return fx;
}

///
EffectBus::EffectBus()
{
    float* buffers = new float[2 * EffectChannels * config::maxBlockSize];
    _buffers.reset(buffers);

    for (unsigned c = 0; c < EffectChannels; ++c) {
        _inputs[c] = &buffers[c * config::maxBlockSize];
        _outputs[c] = &buffers[(c + EffectChannels) * config::maxBlockSize];
    }
}

void EffectBus::addEffect(std::unique_ptr<Effect> fx)
{
    _effects.emplace_back(std::move(fx));
}

void EffectBus::clearInputs(unsigned nframes)
{
    std::memset(_buffers.get(), 0, EffectChannels * sizeof(float) * nframes);
}

void EffectBus::addToInputs(const float* addInput[], float addGain, unsigned nframes)
{
    if (addGain == 0)
        return;

    for (unsigned c = 0; c < EffectChannels; ++c) {
        const float* addIn = addInput[c];
        float* mixedIn = _inputs[c];
        for (unsigned i = 0; i < nframes; ++i)
            mixedIn[i] += addGain * addIn[i];
    }
}

void EffectBus::init(double sampleRate)
{
    for (const auto& effectPtr : _effects)
        effectPtr->init(sampleRate);
}

void EffectBus::clear()
{
    for (const auto& effectPtr : _effects)
        effectPtr->clear();
}

void EffectBus::process(unsigned nframes)
{
    const float** inputs = const_cast<const float**>(_inputs.data());
    float** outputs = _outputs.data();

    size_t numEffects = _effects.size();

    if (numEffects > 0 && hasNonZeroOutput()) {
        _effects[0]->process(inputs, outputs, nframes);
        for (size_t i = 1; i < numEffects; ++i) {
            const float **current = const_cast<const float**>(outputs);
            _effects[i]->process(current, outputs, nframes);
        }
    }
    else
        fx::Nothing{}.process(inputs, outputs, nframes);
}

void EffectBus::mixOutputsTo(float *mainOutput[], float *mixOutput[], unsigned nframes)
{
    const float gainToMain = _gainToMain;
    const float gainToMix = _gainToMix;

    for (unsigned c = 0; c < EffectChannels; ++c) {
        float* fxOut = _outputs[c];
        float* mainOut = mainOutput[c];
        float* mixOut = mixOutput[c];

        for (unsigned i = 0; i < nframes; ++i) {
            mainOut[i] += gainToMain * fxOut[i];
            mixOut[i] += gainToMix * fxOut[i];
        }
    }
}

} // namespace sfz
