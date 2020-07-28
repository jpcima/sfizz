// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "LFO.h"
#include "../../LFO.h"
#include "../../Synth.h"
#include "../../Voice.h"
#include "../../SIMDHelpers.h"
#include "../../Config.h"
#include "../../Debug.h"

namespace sfz {

struct LFOSource::Impl {
    Synth* synth_ = nullptr;
};

LFOSource::LFOSource(Synth &synth)
    : impl_(new Impl)
{
    impl_->synth_ = &synth;
}

LFOSource::~LFOSource()
{
}

void LFOSource::setSampleRate(double sampleRate)
{
    (void)sampleRate;
}

void LFOSource::setSamplesPerBlock(unsigned count)
{
    (void)count;
}

void LFOSource::init(const ModKey& sourceKey, NumericId<Voice> voiceId)
{
    Impl& impl = *impl_;
    Synth& synth = *impl.synth_;
    unsigned lfoIndex = sourceKey.parameters().N;

    Voice* voice = synth.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        return;
    }

    const Region* region = voice->getRegion();
    if (lfoIndex >= region->lfos.size()) {
        ASSERTFALSE;
        return;
    }

    LFO* lfo = voice->getLFO(lfoIndex);
    lfo->configure(&region->lfos[lfoIndex]);
    lfo->start();
}

void LFOSource::generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer)
{
    Impl& impl = *impl_;
    Synth& synth = *impl.synth_;
    const unsigned lfoIndex = sourceKey.parameters().N;

    Voice* voice = synth.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        fill(buffer, 0.0f);
        return;
    }

    const Region* region = voice->getRegion();
    if (lfoIndex >= region->lfos.size()) {
        ASSERTFALSE;
        fill(buffer, 0.0f);
        return;
    }

    LFO* lfo = voice->getLFO(lfoIndex);
    lfo->process(buffer);
}

} // namespace sfz
