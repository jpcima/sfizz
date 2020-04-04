// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Effects.h"
#include <memory>
class faustPhaserStage;

namespace sfz {
namespace fx {

    /**
     * @brief Dual-notch phaser
     */
    class Phaser : public Effect {
    public:
        Phaser();
        ~Phaser();

        /**
         * @brief Initializes with the given sample rate.
         */
        void setSampleRate(double sampleRate) override;

        /**
         * @brief Sets the maximum number of frames to render at a time. The actual
         * value can be lower but should never be higher.
         */
        void setSamplesPerBlock(int samplesPerBlock) override;

        /**
         * @brief Reset the state to initial.
         */
        void clear() override;

        /**
         * @brief Copy the input signal to the output
         */
        void process(const float* const inputs[], float* const outputs[], unsigned nframes) override;

        /**
          * @brief Instantiates given the contents of the <effect> block.
          */
        static std::unique_ptr<Effect> makeInstance(absl::Span<const Opcode> members);

    private:
        enum { MaximumPhaserStages = 4 };

        float _samplePeriod = 0;

        // control
        unsigned _phaserNumStages = 1;
        float _phaserWet = 0.0f;
        float _phaserDepth = 0.0f;
        float _phaserFeedback = 0.0f;
        int _lfoWave = 0;
        float _lfoFrequency = 0.0f;

        // state
        float _lfoPhase = 0;
        std::unique_ptr<faustPhaserStage[]> _phaserStages;

        // buffer
        AudioBuffer<float, 4> _tempBuffer { 4, config::defaultSamplesPerBlock };
    };

} // namespace fx
} // namespace sfz
