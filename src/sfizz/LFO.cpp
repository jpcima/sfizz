// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "LFO.h"
#include "LFODescription.h"
#include "MathHelpers.h"
#include "SIMDHelpers.h"
#include "Config.h"
#include <array>
#include <algorithm>
#include <cmath>

namespace sfz {

struct LFO::Impl {
    float sampleRate_ = 0;

    // control
    const LFODescription* desc_ = nullptr;

    // state
    size_t delayFramesLeft_ = 0;
    float fadeInPole_ = 0;
    float fadeInMemory_ = 0;
    std::array<float, config::maxLFOSubs> subPhases_ {};
    std::array<float, config::maxLFOSubs> sampleHoldMem_ {};
};

LFO::LFO()
    : impl_(new Impl)
{
    impl_->sampleRate_ = config::defaultSampleRate;
    impl_->desc_ = &LFODescription::getDefault();
}

LFO::~LFO()
{
}

void LFO::setSampleRate(double sampleRate)
{
    impl_->sampleRate_ = sampleRate;
}

void LFO::configure(const LFODescription* desc)
{
    impl_->desc_ = desc ? desc : &LFODescription::getDefault();
}

void LFO::start()
{
    Impl& impl = *impl_;
    const LFODescription& desc = *impl.desc_;
    const float sampleRate = impl.sampleRate_;

    impl.subPhases_.fill(desc.phase0);
    impl.sampleHoldMem_.fill(0.0f);

    const float delay = desc.delay;
    impl.delayFramesLeft_ = (delay > 0) ? static_cast<size_t>(std::ceil(sampleRate * delay)) : 0u;

    const float fade = desc.fade;
    impl.fadeInPole_ = (fade > 0) ? std::exp(-1.0 / (fade * sampleRate)) : 0.0f;
    impl.fadeInMemory_ = 0;
}

template <>
inline float LFO::eval<LFOWave::Triangle>(float phase)
{
    float y = -4 * phase + 2;
    y = (phase < 0.25f) ? (4 * phase) : y;
    y = (phase > 0.75f) ? (4 * phase - 4) : y;
    return y;
}

template <>
inline float LFO::eval<LFOWave::Sine>(float phase)
{
    float x = phase + phase - 1;
    return 4 * x * (1 - std::fabs(x));
}

template <>
inline float LFO::eval<LFOWave::Pulse75>(float phase)
{
    return (phase < 0.75f) ? +1.0f : -1.0f;
}

template <>
inline float LFO::eval<LFOWave::Square>(float phase)
{
    return (phase < 0.5f) ? +1.0f : -1.0f;
}

template <>
inline float LFO::eval<LFOWave::Pulse25>(float phase)
{
    return (phase < 0.25f) ? +1.0f : -1.0f;
}

template <>
inline float LFO::eval<LFOWave::Pulse12_5>(float phase)
{
    return (phase < 0.125f) ? +1.0f : -1.0f;
}

template <>
inline float LFO::eval<LFOWave::Ramp>(float phase)
{
    return 2 * phase - 1;
}

template <>
inline float LFO::eval<LFOWave::Saw>(float phase)
{
    return 1 - 2 * phase;
}

template <LFOWave W>
void LFO::processWave(unsigned nth, absl::Span<float> out)
{
    Impl& impl = *impl_;
    const LFODescription& desc = *impl.desc_;
    const LFODescription::Sub& sub = desc.sub[nth];
    const size_t numFrames = out.size();

    float samplePeriod = 1.0f / impl.sampleRate_;
    float phase = impl.subPhases_[nth];
    float baseFreq = desc.freq;
    float offset = sub.offset;
    float ratio = sub.ratio;
    float scale = sub.scale;

    for (size_t i = 0; i < numFrames; ++i) {
        out[i] += offset + scale * eval<W>(phase);

        // TODO(jpc) lfoN_count: number of repetitions

        float incrPhase = ratio * samplePeriod * baseFreq;
        phase += incrPhase;
        int numWraps = (int)phase;
        phase -= numWraps;
    }

    impl.subPhases_[nth] = phase;
}

template <LFOWave W>
void LFO::processSH(unsigned nth, absl::Span<float> out)
{
    Impl& impl = *impl_;
    const LFODescription& desc = *impl.desc_;
    const LFODescription::Sub& sub = desc.sub[nth];
    const size_t numFrames = out.size();

    float samplePeriod = 1.0f / impl.sampleRate_;
    float phase = impl.subPhases_[nth];
    float baseFreq = desc.freq;
    float offset = sub.offset;
    float ratio = sub.ratio;
    float scale = sub.scale;
    float sampleHoldValue = impl.sampleHoldMem_[nth];

    for (size_t i = 0; i < numFrames; ++i) {
        out[i] += offset + scale * sampleHoldValue;

        // TODO(jpc) lfoN_count: number of repetitions

        float incrPhase = ratio * samplePeriod * baseFreq;

        // value updates twice every period
        bool updateValue = (int)(phase * 2.0) != (int)((phase + incrPhase) * 2.0);

        phase += incrPhase;
        int numWraps = (int)phase;
        phase -= numWraps;

        if (updateValue) {
            std::uniform_real_distribution<float> dist(-1.0f, +1.0f);
            sampleHoldValue = dist(Random::randomGenerator);
        }
    }

    impl.subPhases_[nth] = phase;
    impl.sampleHoldMem_[nth] = sampleHoldValue;
}

void LFO::processSteps(absl::Span<float> out)
{
    unsigned nth = 0;
    Impl& impl = *impl_;
    const LFODescription& desc = *impl.desc_;
    const LFODescription::Sub& sub = desc.sub[nth];
    const size_t numFrames = out.size();

    const LFODescription::StepSequence& seq = *desc.seq;
    const float* steps = seq.steps.data();
    unsigned numSteps = seq.steps.size();

    if (numSteps <= 0)
        return;

    float samplePeriod = 1.0f / impl.sampleRate_;
    float phase = impl.subPhases_[nth];
    float baseFreq = desc.freq;
    float offset = sub.offset;
    float ratio = sub.ratio;
    float scale = sub.scale;

    for (size_t i = 0; i < numFrames; ++i) {
        float step = steps[static_cast<int>(phase * numSteps)];
        out[i] += offset + scale * step;

        // TODO(jpc) lfoN_count: number of repetitions

        float incrPhase = ratio * samplePeriod * baseFreq;
        phase += incrPhase;
        int numWraps = (int)phase;
        phase -= numWraps;
    }

    impl.subPhases_[nth] = phase;
}

void LFO::process(absl::Span<float> out)
{
    Impl& impl = *impl_;
    const LFODescription& desc = *impl.desc_;
    size_t numFrames = out.size();

    fill(out, 0.0f);

    size_t skipFrames = std::min(numFrames, impl.delayFramesLeft_);
    if (skipFrames > 0) {
        impl.delayFramesLeft_ -= skipFrames;
        out.remove_prefix(skipFrames);
        numFrames -= skipFrames;
    }

    unsigned subno = 0;
    const unsigned countSubs = desc.sub.size();

    if (countSubs < 1)
        return;

    if (desc.seq) {
        processSteps(out);
        ++subno;
    }

    for (; subno < countSubs; ++subno) {
        switch (desc.sub[subno].wave) {
        case LFOWave::Triangle:
            processWave<LFOWave::Triangle>(subno, out);
            break;
        case LFOWave::Sine:
            processWave<LFOWave::Sine>(subno, out);
            break;
        case LFOWave::Pulse75:
            processWave<LFOWave::Pulse75>(subno, out);
            break;
        case LFOWave::Square:
            processWave<LFOWave::Square>(subno, out);
            break;
        case LFOWave::Pulse25:
            processWave<LFOWave::Pulse25>(subno, out);
            break;
        case LFOWave::Pulse12_5:
            processWave<LFOWave::Pulse12_5>(subno, out);
            break;
        case LFOWave::Ramp:
            processWave<LFOWave::Ramp>(subno, out);
            break;
        case LFOWave::Saw:
            processWave<LFOWave::Saw>(subno, out);
            break;
        case LFOWave::RandomSH:
            processSH<LFOWave::RandomSH>(subno, out);
            break;
        }
    }

    float fadeIn = impl.fadeInMemory_;
    const float fadeInPole = impl.fadeInPole_;
    for (size_t i = 0; i < numFrames; ++i) {
        out[i] *= fadeIn;
        fadeIn = fadeInPole * fadeIn + (1 - fadeInPole);
    }
    impl.fadeInMemory_ = fadeIn;
}

} // namespace sfz
