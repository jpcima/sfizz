// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ADSR.h"
#include "../ModId.h"
#include "../../ADSREnvelope.h"
#include "../../Synth.h"
#include "../../Voice.h"
#include "../../SIMDHelpers.h"
#include "../../Config.h"
#include "../../Debug.h"

namespace sfz {

ADSRSource::ADSRSource(Synth &synth)
    : synth_(&synth)
{
}

void ADSRSource::init(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay)
{
    Synth& synth = *synth_;

    Voice* voice = synth.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        return;
    }

    const Region* region = voice->getRegion();

    ADSREnvelope<float>* adsr;
    const EGDescription* desc;

    std::tie(adsr, desc) = getEnvelopeFromKey(voice, region, sourceKey);
    if (!adsr) {
        ASSERTFALSE;
        return;
    }

    adsr->reset(*desc, *region, synth.getResources().midiState, delay, voice->getTriggerValue(), synth.getSampleRate());
}

void ADSRSource::release(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay)
{
    Synth& synth = *synth_;

    Voice* voice = synth.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        return;
    }

    const Region* region = voice->getRegion();

    ADSREnvelope<float>* adsr;
    const EGDescription* desc;

    std::tie(adsr, desc) = getEnvelopeFromKey(voice, region, sourceKey);
    if (!adsr) {
        ASSERTFALSE;
        return;
    }

    adsr->startRelease(delay);
}

void ADSRSource::generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer)
{
    Synth& synth = *synth_;

    Voice* voice = synth.getVoiceById(voiceId);
    if (!voice) {
        ASSERTFALSE;
        return;
    }

    const Region* region = voice->getRegion();

    ADSREnvelope<float>* adsr;
    const EGDescription* desc;

    std::tie(adsr, desc) = getEnvelopeFromKey(voice, region, sourceKey);
    if (!adsr) {
        ASSERTFALSE;
        return;
    }

    adsr->getBlock(buffer);
}

std::pair<ADSREnvelope<float>*, const EGDescription*> ADSRSource::getEnvelopeFromKey(Voice* voice, const Region* region, const ModKey& sourceKey)
{
    switch (sourceKey.id()) {
    default:
        return { nullptr, nullptr };
    case ModId::AmpEG:
        return { &voice->getAmplitudeEG(), &region->amplitudeEG };
    case ModId::PitchEG:
        return { &voice->getPitchEG(), &region->pitchEG };
    case ModId::FilEG:
        return { &voice->getFilterEG(), &region->filterEG };
    }
}

} // namespace sfz
