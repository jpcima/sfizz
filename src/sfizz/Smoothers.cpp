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

PowerFunctionTable::PowerFunctionTable(float y)
{
    // Generate the exponent table for the given y
    for (int i = 0; i < exponentTableSize; i++)
        exponentTable[i] = std::pow(2.0f, (i + FP_traits<float>::e_offset) * y);

    // Generate the mantissa table for the given y
    for (unsigned i = 0; i < tableSize; i++) {
        const double x = 1 + static_cast<double>(i) * tableStep;
        const double start = std::pow(x, static_cast<double>(y));
        mantissaTable[i].start = static_cast<float>(start);
        const double end = std::pow(x + tableStepRange, static_cast<double>(y));
        mantissaTable[i].step = static_cast<float>((end - start) / tableStepRange * mantissaNorm);
    }
}

float PowerFunctionTable::get(float x) const
{
    union {
        float real;
        unsigned int integer;
    } fi;
    fi.real = x;
    const int expIndex = (fi.integer >>FP_traits<float>::m_bits) & mask(FP_traits<float>::e_bits);
    const unsigned int b = fi.integer & mask(FP_traits<float>::m_bits);
    const unsigned int b1 = b & (mask(config::powerTableSizeExponent) << lerpBits);
    const int idx = b1 >> lerpBits;
    const unsigned int b2 = b & mask(lerpBits);
    return exponentTable[expIndex] * (mantissaTable[idx].start + static_cast<float>(b2) * mantissaTable[idx].step);
}

LinearSmoother::LinearSmoother()
{
    setSmoothing(0, config::defaultSampleRate);
}

void LinearSmoother::setSmoothing(uint8_t smoothValue, float sampleRate)
{
    static const auto smoothToGainTable = [] {
        std::array<float, config::smoothingSteps> gains;
        for (uint8_t i = 0; i < config::smoothingSteps; ++i)
            gains[i] = std::tan(pi<float>() / (smoothTableRange.getStart() + i * smoothTableStep));
        return gains;
    }();

    smoothing = (smoothValue > 0);
    if (smoothing) {
        const auto tableIndex = smoothTableIndex(smoothValue, sampleRate);
        filter.setGain(smoothToGainTable[tableIndex]);
    }
}

void LinearSmoother::reset(float value)
{
    filter.reset(value);
}

void LinearSmoother::process(absl::Span<const float> input, absl::Span<float> output)
{
    if (smoothing)
        filter.processLowpass(input, output);
    else
        copy<float>(input, output);
}

MultiplicativeSmoother::MultiplicativeSmoother()
{
    setSmoothing(0, config::defaultSampleRate);
}

void MultiplicativeSmoother::setSmoothing(uint8_t smoothValue, float sampleRate)
{
    static const auto smoothToPowerFunctionTable = [] {
        std::vector<PowerFunctionTable> powerFunctions;

        for (uint8_t i = 0; i < config::smoothingSteps; ++i) {
            const auto gain = std::tan(pi<float>() / (smoothTableRange.getStart() + i * smoothTableStep));
            const auto G = gain / (1 - gain);
            powerFunctions.push_back(PowerFunctionTable(G));
        }
        return powerFunctions;
    }();

    smoothing = (smoothValue > 0);
    if (smoothing) {
        const auto tableIndex = smoothTableIndex(smoothValue, sampleRate);
        gainPower = &smoothToPowerFunctionTable[tableIndex];
    }
}

void MultiplicativeSmoother::reset(float value)
{
    state = value;
}

void MultiplicativeSmoother::process(absl::Span<const float> input, absl::Span<float> output)
{
    if (!smoothing) {
        copy<float>(input, output);
        return;
    }

    CHECK_SPAN_SIZES(input, output);
    const auto size = minSpanSize(input, output);
    for (unsigned i = 0; i < size; ++i) {
        const auto intermediate = gainPower->get(input[i] / state);
        output[i] = intermediate * state;
        state = output[i] * intermediate;
    }
}

}
