// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/*
   Note(jpc): implementation status

- [x] phaser_wet
- [ ] phaser_wet_onccN
- [x] phaser_depth
- [ ] phaser_depth_onccN
- [x] phaser_stages
- [x] phaser_feedback
- [ ] phaser_feedback_onccN
- [x] phaser_waveform
- [x] phaser_freq
- [ ] phaser_freq_onccN
- [ ] phaser_phase_onccN
*/

#include "Phaser.h"
#include "CommonLFO.h"
#include "SIMDHelpers.h"
#include "Opcode.h"
#include "gen/phaser_stage.cpp"
#include <cmath>

namespace sfz {
namespace fx {

    Phaser::Phaser()
        : _phaserStages(new faustPhaserStage[EffectChannels * MaximumPhaserStages])
    {
    }

    Phaser::~Phaser()
    {
    }

    void Phaser::setSampleRate(double sampleRate)
    {
        _samplePeriod = 1.0 / sampleRate;

        faustPhaserStage* stages = _phaserStages.get();
        for (unsigned i = 0, n = EffectChannels * _phaserNumStages; i < n; ++i) {
            stages[i].classInit(sampleRate);
            stages[i].instanceConstants(sampleRate);
            stages[i].instanceClear();
        }
    }

    void Phaser::setSamplesPerBlock(int samplesPerBlock)
    {
        _tempBuffer.resize(samplesPerBlock);
    }

    void Phaser::clear()
    {
        _lfoPhase = 0;

        faustPhaserStage* stages = _phaserStages.get();
        for (unsigned i = 0, n = EffectChannels * _phaserNumStages; i < n; ++i)
            stages[i].instanceClear();
    }

    void Phaser::process(const float* const inputs[], float* const outputs[], unsigned nframes)
    {
        absl::Span<float> lfo = _tempBuffer.getSpan(0).first(nframes);
        absl::Span<float> depth = _tempBuffer.getSpan(1).first(nframes);
        absl::Span<float> feedback = _tempBuffer.getSpan(2).first(nframes);
        absl::Span<float> wet = _tempBuffer.getSpan(3).first(nframes);

        float lfoPhase = _lfoPhase;
        const float lfoFrequency = _lfoFrequency;
        const float samplePeriod = _samplePeriod;

        switch (_lfoWave) {
        #define CASE(X) case lfo::X:                                \
            for (unsigned i = 0; i < nframes; ++i) {                \
                lfo[i] = lfo::evaluateAtPhase<lfo::X>(lfoPhase);    \
                lfoPhase += lfoFrequency * samplePeriod;            \
                lfoPhase -= (int)lfoPhase;                          \
            }                                                       \
            break;
        default:
        CASE(kTriangle)
        CASE(kSine)
        CASE(kPulse75)
        CASE(kSquare)
        CASE(kPulse25)
        CASE(kPulse12_5)
        CASE(kRamp)
        CASE(kSaw)
        #undef CASE
        }

        sfz::fill(depth, _phaserDepth);
        sfz::fill(feedback, _phaserFeedback);
        sfz::fill(wet, _phaserWet);
        // TODO(jpc) phaser_depth_onccN, phaser_feedback_onccN, phaser_wet_onccN

        const unsigned numStages = _phaserNumStages;

        for (unsigned c = 0; c < EffectChannels; ++c) {
            auto input = absl::MakeConstSpan(inputs[c], nframes);
            auto output = absl::MakeSpan(outputs[c], nframes);
            faustPhaserStage* stages = &_phaserStages[c * numStages];

            // compute stage 1: input -> output
            {
                const float* stageInputs[] = {lfo.data(), depth.data(), feedback.data(), input.data()};
                float* stageOutputs[] = {output.data()};
                stages[0].compute(nframes, const_cast<float**>(stageInputs), stageOutputs);
            }
            // compute stages 2-N: output -> output
            for (unsigned s = 1; s < numStages; ++s) {
                const float* stageInputs[] = {lfo.data(), depth.data(), feedback.data(), output.data()};
                float* stageOutputs[] = {output.data()};
                stages[s].compute(nframes, const_cast<float**>(stageInputs), stageOutputs);
            }

            // mix
            for (unsigned i = 0; i < nframes; ++i) {
                float w = wet[i] * 0.01f;
                output[i] = output[i] * w + input[i] * (1.0f - w);
            }
        }

        _lfoPhase = lfoPhase;
    }

    std::unique_ptr<Effect> Phaser::makeInstance(absl::Span<const Opcode> members)
    {
        std::unique_ptr<Phaser> fx { new Phaser };

        for (const Opcode& opc : members) {
            switch (opc.lettersOnlyHash) {
            case hash("phaser_stages"):
                setValueFromOpcode(opc, fx->_phaserNumStages, {1, MaximumPhaserStages});
                break;
            case hash("phaser_depth"):
                setValueFromOpcode(opc, fx->_phaserDepth, Default::phaserDepthRange);
                break;
            case hash("phaser_wet"):
                setValueFromOpcode(opc, fx->_phaserWet, Default::phaserLevelRange);
                break;
            case hash("phaser_feedback"):
                setValueFromOpcode(opc, fx->_phaserFeedback, Default::phaserFeedbackRange);
                break;
            case hash("phaser_waveform"):
                if (auto value = readOpcode(opc.value, Default::phaserWaveformRange))
                    fx->_lfoWave = *value;
                break;
            case hash("phaser_freq"):
                setValueFromOpcode(opc, fx->_lfoFrequency, Default::phaserFrequencyRange);
                break;
            }
        }

        return CXX11_MOVE(fx);
    }

} // namespace fx
} // namespace sfz
