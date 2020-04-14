#pragma once

#include "Config.h"
#include "Defaults.h"
#include "MathHelpers.h"
#include "SfzHelpers.h"
#include "OnePoleFilter.h"
#include "SIMDHelpers.h"
#include <array>

namespace sfz {
class PowerFunctionTable {
public:
    PowerFunctionTable() = delete;
    PowerFunctionTable(float y);
    float get(float x) const;
private:
    static constexpr int exponentTableSize = 1 << FP_traits<float>::e_bits;
    static constexpr unsigned tableSize = 1U << config::powerTableSizeExponent;
    static constexpr unsigned lerpBits = FP_traits<float>::m_bits - config::powerTableSizeExponent;
    static constexpr double tableStep = 1 / static_cast<double>(tableSize);
    static constexpr double mantissaEps = 1 / static_cast<double>(1U << (FP_traits<float>::m_bits + 1));
    static constexpr double tableStepRange = tableStep - mantissaEps;
    static constexpr double mantissaNorm = static_cast<double>(1.0 / (1U << FP_traits<float>::m_bits));
    std::array<float, exponentTableSize> exponentTable;
    struct Entries {
        float start;
        float step;
    };
    std::array<Entries, tableSize> mantissaTable;
};

class LinearSmoother {
public:
    LinearSmoother();
    void setSmoothing(uint8_t smoothValue, float sampleRate);
    void reset(float value = 0.0f);
    void process(absl::Span<const float> input, absl::Span<float> output);
private:
    bool smoothing { false };
    OnePoleFilter<float> filter;
};

class MultiplicativeSmoother {
public:
    MultiplicativeSmoother();
    void setSmoothing(uint8_t smoothValue, float sampleRate);
    void reset(float value = 1.0f);
    void process(absl::Span<const float> input, absl::Span<float> output);
private:
    bool smoothing { false };
    float state { 1.0 };
    const PowerFunctionTable* gainPower;
    static constexpr float eps = 1e-5;
};

}
