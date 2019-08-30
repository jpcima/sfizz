#include "Helpers.h"
#include "SIMDHelpers.h"

#include <arm_neon.h>

using Type = float;
[[maybe_unused]] constexpr uintptr_t TypeAlignment { 4 };
[[maybe_unused]] constexpr uintptr_t TypeAlignmentMask { TypeAlignment - 1 };
[[maybe_unused]] constexpr uintptr_t ByteAlignment { TypeAlignment * sizeof(Type) };
[[maybe_unused]] constexpr uintptr_t ByteAlignmentMask { ByteAlignment - 1 };

struct AlignmentSentinels {
    float* nextAligned;
    float* lastAligned;
};

float* nextAligned(const float* ptr)
{
    return reinterpret_cast<float*>((reinterpret_cast<uintptr_t>(ptr) + ByteAlignmentMask) & (~ByteAlignmentMask));
}

float* prevAligned(const float* ptr)
{
    return reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(ptr) & (~ByteAlignmentMask));
}

bool unaligned(const float* ptr)
{
    return (reinterpret_cast<uintptr_t>(ptr) & ByteAlignmentMask) != 0;
}

bool unaligned(const float* ptr1, const float* ptr2)
{
    return unaligned(ptr1) || unaligned(ptr2);
}

bool unaligned(const float* ptr1, const float* ptr2, const float* ptr3)
{
    return unaligned(ptr1) || unaligned(ptr2) || unaligned(ptr3);
}

bool unaligned(const float* ptr1, const float* ptr2, const float* ptr3, const float* ptr4)
{
    return unaligned(ptr1) || unaligned(ptr2) || unaligned(ptr3) || unaligned(ptr4);
}

template <>
void readInterleaved<float, true>(absl::Span<const float> input, absl::Span<float> outputLeft, absl::Span<float> outputRight) noexcept
{
    readInterleaved<float, false>(input, outputLeft, outputRight);
}

template <>
void writeInterleaved<float, true>(absl::Span<const float> inputLeft, absl::Span<const float> inputRight, absl::Span<float> output) noexcept
{
    writeInterleaved<float, false>(inputLeft, inputRight, output);
}

template <>
void fill<float, true>(absl::Span<float> output, float value) noexcept
{
    const auto vecValue = vmovq_n_f32(value);
    auto* out = output.begin();
    const auto* lastAligned = prevAligned(output.end());

    while (unaligned(out) && out < lastAligned)
        *out++ = value;

    while (out < lastAligned) // we should only need to test a single channel
    {
        vst1q_f32(out, vecValue);
        out += TypeAlignment;
    }

    while (out < output.end())
        *out++ = value;
    
}

template <>
void exp<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    exp<float, false>(input, output);
}

template <>
void log<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    log<float, false>(input, output);
}

template <>
void sin<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    sin<float, false>(input, output);
}

template <>
void cos<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    cos<float, false>(input, output);
}

template <>
void applyGain<float, true>(float gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    applyGain<float, false>(gain, input, output);
}

template <>
void applyGain<float, true>(absl::Span<const float> gain, absl::Span<const float> input, absl::Span<float> output) noexcept
{
    applyGain<float, false>(gain, input, output);
}

template <>
float loopingSFZIndex<float, true>(absl::Span<const float> jumps, absl::Span<float> leftCoeff, absl::Span<float> rightCoeff, absl::Span<int> indices, float floatIndex, float loopEnd, float loopStart) noexcept
{
    return loopingSFZIndex<float, false>(jumps, leftCoeff, rightCoeff, indices, floatIndex, loopEnd, loopStart);
}

template <>
float saturatingSFZIndex<float, true>(absl::Span<const float> jumps, absl::Span<float> leftCoeff, absl::Span<float> rightCoeff, absl::Span<int> indices, float floatIndex, float loopEnd) noexcept
{
    return saturatingSFZIndex<float, false>(jumps, leftCoeff, rightCoeff, indices, floatIndex, loopEnd);
}


template <>
float linearRamp<float, true>(absl::Span<float> output, float start, float step) noexcept
{
    return linearRamp<float, false>(output, start, step);
}

template <>
float multiplicativeRamp<float, true>(absl::Span<float> output, float start, float step) noexcept
{
    return multiplicativeRamp<float, false>(output, start, step);
}

template <>
void add<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    add<float, false>(input, output);
}
