#include "Smoothers.h"

namespace sfz {

constexpr Range<float> smoothTableRange {
    config::defaultSampleRate * Default::smoothTauPerStep,
    Default::smoothCCRange.getEnd() * config::maxSampleRate * Default::smoothTauPerStep
};

constexpr auto smoothTableStep = smoothTableRange.length() / (config::smoothingSteps - 1);

constexpr float smoothTableClamp(uint8_t smooth, float sampleRate)
{
    return smoothTableRange.clamp(sampleRate * smooth * Default::smoothTauPerStep);
}

constexpr int smoothTableIndex(uint8_t smooth, float sampleRate)
{
    return static_cast<int>((smoothTableClamp(smooth, sampleRate) - smoothTableRange.getStart()) / smoothTableStep);
}

Smoother::Smoother()
{
    setSmoothing(0, config::defaultSampleRate);
}

void Smoother::setSmoothing(uint8_t smoothValue, float sampleRate)
{
    static const auto smoothToGainTable = [] {
        std::array<float, config::smoothingSteps> gains;
        for (unsigned i = 0; i < config::smoothingSteps; ++i)
            gains[i] = std::tan(pi<float>() / (smoothTableRange.getStart() + i * smoothTableStep));
        return gains;
    }();

    smoothing = (smoothValue > 0);
    if (smoothing) {
        const auto tableIndex = smoothTableIndex(smoothValue, sampleRate);
        filter.setGain(smoothToGainTable[tableIndex]);
    }
}

void Smoother::reset(float value)
{
    filter.reset(value);
}

void Smoother::process(absl::Span<const float> input, absl::Span<float> output)
{
    if (smoothing)
        filter.processLowpass(input, output);
    else
        copy<float>(input, output);
}

}
