// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "../ModGenerator.h"

namespace sfz {
class Synth;
struct Region;
struct EGDescription;
template <class T> class ADSREnvelope;

class ADSRSource : public ModGenerator {
public:
    explicit ADSRSource(Synth &synth);
    void init(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay) override;
    void release(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay) override;
    void generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer) override;

private:
    std::pair<ADSREnvelope<float>*, const EGDescription*> getEnvelopeFromKey(Voice* voice, const Region* region, const ModKey& sourceKey);

private:
    Synth* synth_ = nullptr;
};

} // namespace sfz
