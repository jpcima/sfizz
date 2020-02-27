// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include <array>
#include <vector>
#include <memory>

namespace sfz {
class Opcode;

enum {
    // Number of channels processed by effects
    EffectChannels = 2,
};

/**
   @brief Abstract base of SFZ effects
 */
class Effect {
public:
    virtual ~Effect() {}

    /**
       @brief Initializes with the given sample rate.
     */
    virtual void init(double sampleRate) = 0;

    /**
       @brief Reset the state to initial.
     */
    virtual void clear() = 0;

    /**
       @brief Computes a cycle of the effect in stereo.
     */
    virtual void process(const float* inputs[], float* outputs[], unsigned nframes) = 0;

    /**
       @brief Type of the factory function used to instantiate an effect given
              the contents of the <effect> block
     */
    typedef Effect* (MakeInstance)(absl::Span<const Opcode> members);
};

/**
   @brief SFZ effects factory
 */
class EffectFactory {
public:
    /**
       @brief Registers all available standard effects into the factory.
     */
    void registerStandardEffectTypes();

    /**
       @brief Registers a user-defined effect into the factory.
     */
    void registerEffectType(absl::string_view name, Effect::MakeInstance& make);

    /**
       @brief Instantiates an effect given the contents of the <effect> block.
     */
    Effect* makeEffect(absl::Span<const Opcode> members);

private:
    struct FactoryEntry {
        std::string name;
        Effect::MakeInstance* make;
    };

    std::vector<FactoryEntry> _entries;
};

/**
   @brief Sequence of effects processed in series
 */
class EffectBus {
public:
    EffectBus();

    /**
       @brief Adds an effect at the end of the bus.
     */
    void addEffect(std::unique_ptr<Effect> fx);

    /**
       @brief Checks whether this bus can produce output.
     */
    bool hasNonZeroOutput() const { return _gainToMain != 0 && _gainToMix != 0; }

    /**
       @brief Sets the amount of effect output going to the main.
     */
    void setGainToMain(float gain) { _gainToMain = gain; }

    /**
       @brief Sets the amount of effect output going to the mix.
     */
    void setGainToMix(float gain) { _gainToMix = gain; }

    /**
       @brief Resets the input buffers to zero.
     */
    void clearInputs(unsigned nframes);

    /**
       @brief Adds some audio into the input buffer.
     */
    void addToInputs(const float* addInput[], float addGain, unsigned nframes);

    /**
       @brief Initializes all effects in the bus with the given sample rate.
     */
    void init(double sampleRate);

    /**
       @brief Resets the state of all effects in the bus.
     */
    void clear();

    /**
       @brief Computes a cycle of the effect bus.
     */
    void process(unsigned nframes);

    /**
       @brief Mixes the outputs into a pair of stereo signals: Main and Mix.
     */
    void mixOutputsTo(float *mainOutput[], float *mixOutput[], unsigned nframes);

private:
    std::vector<std::unique_ptr<Effect>> _effects;
    std::array<float*, EffectChannels> _inputs;
    std::array<float*, EffectChannels> _outputs;
    std::unique_ptr<float[]> _buffers;
    float _gainToMain = 0.0;
    float _gainToMix = 0.0;
};

} // namespace sfz
