#pragma once
#include "SfzFilter.h"
#include "Region.h"
#include "Resources.h"
#include "Defaults.h"
#include "utility/SpinMutex.h"
#include <vector>
#include <memory>
#include <mutex>

namespace sfz
{

class FilterHolder
{
public:
    FilterHolder() = delete;
    FilterHolder(Resources& resources);
    /**
     * @brief Setup a new filter based on a filter description, and a triggering note parameters.
     *
     * @param description   the region from which we take the filter
     * @param filterId      the filter index in the region
     * @param noteNumber    the triggering note number
     * @param velocity      the triggering note velocity/value
     */
    void setup(const Region& region, unsigned filterId, int noteNumber = static_cast<int>(Default::key.value), float velocity = 0);
    /**
     * @brief Process a block of stereo inputs
     *
     * @param inputs
     * @param outputs
     * @param numFrames
     */
    void process(const float** inputs, float** outputs, unsigned numFrames);
    /**
     * @brief Set the sample rate for a filter
     *
     * @param sampleRate
     */
    void setSampleRate(float sampleRate);
    /**
     * Reset the filter.
     */
    void reset();
private:
    Resources& resources;
    const FilterDescription* description;
    std::unique_ptr<Filter> filter;
    float baseCutoff { Default::filterCutoff.value };
    float baseResonance { Default::filterResonance.value };
    float baseGain { Default::filterGain.value };
    ModMatrix::TargetId gainTarget;
    ModMatrix::TargetId cutoffTarget;
    ModMatrix::TargetId resonanceTarget;
    bool prepared { false };
};

}
