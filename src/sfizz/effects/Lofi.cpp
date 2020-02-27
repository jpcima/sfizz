// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
   Note(jpc): implementation status

- [x] bitred
- [ ] bitred_oncc
- [ ] bitred_smoothcc
- [ ] bitred_stepcc
- [ ] bitred_curvecc

- [x] decim
- [ ] decim_oncc
- [ ] decim_smoothcc
- [ ] decim_stepcc
- [ ] decim_curvecc

- [ ] egN_bitred
- [ ] egN_bitred_oncc
- [ ] lfoN_bitred
- [ ] lfoN_bitred_oncc
- [ ] lfoN_bitred_smoothcc
- [ ] lfoN_bitred_stepcc

- [ ] egN_decim
- [ ] egN_decim_oncc
- [ ] lfoN_decim
- [ ] lfoN_decim_oncc
- [ ] lfoN_decim_smoothcc
- [ ] lfoN_decim_stepcc

 */

#include "Lofi.h"
#include "Opcode.h"
#include <memory>
#include <algorithm>
#include <cstring>
#include <cmath>

namespace sfz {
namespace fx {

void Lofi::init(double sampleRate)
{
    for (unsigned c = 0; c < EffectChannels; ++c) {
        _bitred[c].init(sampleRate);
        _decim[c].init(sampleRate);
    }
}

void Lofi::clear()
{
    for (unsigned c = 0; c < EffectChannels; ++c) {
        _bitred[c].clear();
        _decim[c].clear();
    }
}

void Lofi::process(const float* inputs[2], float* outputs[2], unsigned nframes)
{
    for (unsigned c = 0; c < EffectChannels; ++c) {
        _bitred[c].setDepth(_bitred_depth);
        _bitred[c].process(inputs[c], outputs[c], nframes);

        _decim[c].setDepth(_decim_depth);
        _decim[c].process(outputs[c], outputs[c], nframes);
    }
}

Effect* Lofi::makeInstance(absl::Span<const Opcode> members)
{
    std::unique_ptr<Lofi> fx { new Lofi };

    for (const Opcode& opcode : members) {
        switch (opcode.lettersOnlyHash) {
        case hash("bitred"):
            setValueFromOpcode(opcode, fx->_bitred_depth, {0.0, 100.0});
            break;
        case hash("decim"):
            setValueFromOpcode(opcode, fx->_decim_depth, {0.0, 100.0});
            break;
        }
    }

    return fx.release();
}

///
inline void Lofi::HPF::init(double sampleRate)
{
    fSampleTime = 1.0 / sampleRate;
}

inline void Lofi::HPF::setCutoff(double frequency)
{
    fPole = std::exp(-2.0 * M_PI * frequency * fSampleTime);
}

inline float Lofi::HPF::process(float input)
{
    float p = fPole;
    float output = input * (0.5f * (1 + p)) + fDelayInput * (-0.5f * (1 + p)) + fDelayOutput * p;
    fDelayInput = input;
    fDelayOutput = output;
    return output;
}

inline void Lofi::HPF::clear()
{
    fDelayInput = 0.0;
    fDelayOutput = 0.0;
}

///
void Lofi::Bitred::init(double sampleRate)
{
    fHpf.init(sampleRate);
    fHpf.setCutoff(20.0);

    static constexpr double coefs2x[12] = { 0.036681502163648017, 0.13654762463195794, 0.27463175937945444, 0.42313861743656711, 0.56109869787919531, 0.67754004997416184, 0.76974183386322703, 0.83988962484963892, 0.89226081800387902, 0.9315419599631839, 0.96209454837808417, 0.98781637073289585 };
    fDownsampler2x.set_coefs(coefs2x);
}

void Lofi::Bitred::clear()
{
    fLastValue = 0.0;
    fDownsampler2x.clear_buffers();
    fHpf.clear();
}

void Lofi::Bitred::setDepth(float depth)
{
    fDepth = std::max(0.0f, std::min(100.0f, depth));
}

void Lofi::Bitred::process(const float* in, float* out, uint32_t nframes)
{
    float depth = fDepth;

    if (depth == 0) {
        if (in != out)
            std::memcpy(out, in, nframes * sizeof(float));
        clear();
        return;
    }

    HPF& hpf = fHpf;
    float lastValue = fLastValue;
    hiir::Downsampler2xFpu<12>& downsampler2x = fDownsampler2x;

    float steps = 5.0f + (100.0f - depth) * 5.12f;

    for (uint32_t i = 0; i < nframes; ++i) {
        float x = in[i];

        float y = std::copysign((int)std::fabs(x * steps), x) * (1 / steps);

        float y2x[2];
        y2x[0] = (y != lastValue) ? (0.5f * (y + lastValue)) : y;
        y2x[1] = y;

        lastValue = y;

        y = downsampler2x.process_sample(y2x);
        y = hpf.process(y);
        out[i] = y;
    }

    fLastValue = lastValue;
}

///
void Lofi::Decim::init(double sampleRate)
{
    fSampleTime = 1.0 / sampleRate;

    fHpf.init(sampleRate);
    fHpf.setCutoff(20.0);

    static constexpr double coefs2x[12] = { 0.036681502163648017, 0.13654762463195794, 0.27463175937945444, 0.42313861743656711, 0.56109869787919531, 0.67754004997416184, 0.76974183386322703, 0.83988962484963892, 0.89226081800387902, 0.9315419599631839, 0.96209454837808417, 0.98781637073289585 };
    fDownsampler2x.set_coefs(coefs2x);
}

void Lofi::Decim::clear()
{
    fPhase = 0.0;
    fLastValue = 0.0;
    fDownsampler2x.clear_buffers();
    fHpf.clear();
}

void Lofi::Decim::setDepth(float depth)
{
    fDepth = std::max(0.0f, std::min(100.0f, depth));
}

void Lofi::Decim::process(const float* in, float* out, uint32_t nframes)
{
    float depth = fDepth;

    if (depth == 0) {
        if (in != out)
            std::memcpy(out, in, nframes * sizeof(float));
        clear();
        return;
    }

    float dt;
    {
        // exponential curve fit
        float a = 5.729950e+04, b = -6.776081e-02, c = 180;
        float freq = a * std::exp(fDepth * b) + c;
        dt = freq * fSampleTime;
    }

    HPF& hpf = fHpf;
    float phase = fPhase;
    float lastValue = fLastValue;
    hiir::Downsampler2xFpu<12>& downsampler2x = fDownsampler2x;

    for (uint32_t i = 0; i < nframes; ++i) {
        float x = in[i];

        phase += dt;
        float y = (phase > 1.0f) ? x : lastValue;
        phase -= (int)phase;

        float y2x[2];
        y2x[0] = (y != lastValue) ? (0.5f * (y + lastValue)) : y;
        y2x[1] = y;

        lastValue = y;

        y = downsampler2x.process_sample(y2x);
        y = hpf.process(y);
        out[i] = y;
    }

    fPhase = phase;
    fLastValue = lastValue;
}

} // namespace fx
} // namespace sfz
