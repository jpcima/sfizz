// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SIMDConfig.h"

#if SFIZZ_HAVE_SSE2

#include "SIMDHelpers.h"
#include <array>
#include <xmmintrin.h>
#include <emmintrin.h>
#include "mathfuns/sse_mathfun.h"

constexpr uintptr_t TypeAlignment = 4;

template <>
void sfz::pan<float, true>(absl::Span<const float> panEnvelope, absl::Span<float> leftBuffer, absl::Span<float> rightBuffer) noexcept
{
    CHECK(leftBuffer.size() >= panEnvelope.size());
    CHECK(rightBuffer.size() >= panEnvelope.size());
    auto* pan = panEnvelope.begin();
    auto* left = leftBuffer.begin();
    auto* right = rightBuffer.begin();
    auto* sentinel = pan + min(panEnvelope.size(), leftBuffer.size(), rightBuffer.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(pan, left, right) && pan < lastAligned) {
        _internals::snippetPan(*pan, *left, *right);
        incrementAll(pan, left, right);
    }

    const auto mmOne = _mm_set_ps1(1.0f);
    const auto mmPiFour = _mm_set_ps1(piFour<float>());
    __m128 mmCos;
    __m128 mmSin;
    while (pan < lastAligned) {
        auto mmPan = _mm_load_ps(pan);
        mmPan = _mm_add_ps(mmOne, mmPan);
        mmPan = _mm_mul_ps(mmPan, mmPiFour);
        sincos_ps(mmPan, &mmSin, &mmCos);
        auto mmLeft = _mm_mul_ps(mmCos, _mm_load_ps(left));
        auto mmRight = _mm_mul_ps(mmSin, _mm_load_ps(right));
        _mm_store_ps(left, mmLeft);
        _mm_store_ps(right, mmRight);
        incrementAll<TypeAlignment>(pan, left, right);
    }

    while (pan < sentinel){
        _internals::snippetPan(*pan, *left, *right);
        incrementAll(pan, left, right);
    }
}

template <>
void sfz::width<float, true>(absl::Span<const float> widthEnvelope, absl::Span<float> leftBuffer, absl::Span<float> rightBuffer) noexcept
{
    CHECK(leftBuffer.size() >= widthEnvelope.size());
    CHECK(rightBuffer.size() >= widthEnvelope.size());
    auto* width = widthEnvelope.begin();
    auto* left = leftBuffer.begin();
    auto* right = rightBuffer.begin();
    auto* sentinel = width + min(widthEnvelope.size(), leftBuffer.size(), rightBuffer.size());
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(width, left, right) && width < lastAligned) {
        _internals::snippetWidth(*width, *left, *right);
        incrementAll(width, left, right);
    }

    const auto mmPiFour = _mm_set_ps1(piFour<float>());
    __m128 mmCos;
    __m128 mmSin;
    while (width < lastAligned) {
        auto mmWidth = _mm_load_ps(width);
        mmWidth = _mm_mul_ps(mmWidth, mmPiFour);
        sincos_ps(mmWidth, &mmSin, &mmCos);
        auto mmCosPlusSine = _mm_add_ps(mmCos, mmSin);
        auto mmCosMinusSine = _mm_sub_ps(mmCos, mmSin);
        auto mmLeft = _mm_load_ps(left);
        auto mmRight = _mm_load_ps(right);
        auto mmTemp = _mm_mul_ps(mmCosMinusSine, mmRight);
        mmRight = _mm_add_ps(_mm_mul_ps(mmCosMinusSine, mmLeft), _mm_mul_ps(mmCosPlusSine, mmRight));
        mmLeft = _mm_add_ps(_mm_mul_ps(mmCosPlusSine, mmLeft), mmTemp);
        _mm_store_ps(left, mmLeft);
        _mm_store_ps(right, mmRight);
        incrementAll<TypeAlignment>(width, left, right);
    }

    while (width < sentinel){
        _internals::snippetWidth(*width, *left, *right);
        incrementAll(width, left, right);
    }
}

template <>
float sfz::mean<float, true>(absl::Span<const float> vector) noexcept
{
    float result { 0.0 };
    if (vector.size() == 0)
        return result;

    auto* value = vector.begin();
    auto* sentinel = vector.end();
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(value) && value < lastAligned)
        result += *value++;

    auto mmSums = _mm_setzero_ps();
    while (value < lastAligned) {
        mmSums = _mm_add_ps(mmSums, _mm_load_ps(value));
        value += TypeAlignment;
    }

    std::array<float, 4> sseResult;
    _mm_store_ps(sseResult.data(), mmSums);

    for (auto sseValue : sseResult)
        result += sseValue;

    while (value < sentinel)
        result += *value++;

    return result / static_cast<float>(vector.size());
}

template <>
float sfz::meanSquared<float, true>(absl::Span<const float> vector) noexcept
{
    float result { 0.0 };
    if (vector.size() == 0)
        return result;

    auto* value = vector.begin();
    auto* sentinel = vector.end();
    const auto* lastAligned = prevAligned(sentinel);

    while (unaligned(value) && value < lastAligned) {
        result += (*value) * (*value);
        value++;
    }

    auto mmSums = _mm_setzero_ps();
    while (value < lastAligned) {
        const auto mmValues = _mm_load_ps(value);
        mmSums = _mm_add_ps(mmSums, _mm_mul_ps(mmValues, mmValues));
        value += TypeAlignment;
    }

    std::array<float, 4> sseResult;
    _mm_store_ps(sseResult.data(), mmSums);

    for (auto sseValue : sseResult)
        result += sseValue;

    while (value < sentinel) {
        result += (*value) * (*value);
        value++;
    }

    return result / static_cast<float>(vector.size());
}

template <>
void sfz::cumsum<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    CHECK(output.size() >= input.size());
    if (input.size() == 0)
        return;

    auto out = output.data();
    auto in = input.data();
    const auto sentinel = in + std::min(input.size(), output.size());
    const auto lastAligned = prevAligned(sentinel);

    *out++ = *in++;
    while (unaligned(in, out) && in < lastAligned)
        _internals::snippetCumsum(in, out);

    auto mmOutput = _mm_set_ps1(*(out - 1));
    while (in < lastAligned) {
        auto mmOffset = _mm_load_ps(in);
        mmOffset = _mm_add_ps(mmOffset, _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(mmOffset), 4)));
        mmOffset = _mm_add_ps(mmOffset, _mm_shuffle_ps(_mm_setzero_ps(), mmOffset, _MM_SHUFFLE(1, 0, 0, 0)));
        mmOutput = _mm_add_ps(mmOutput, mmOffset);
        _mm_store_ps(out, mmOutput);
        mmOutput = _mm_shuffle_ps(mmOutput, mmOutput, _MM_SHUFFLE(3, 3, 3, 3));
        incrementAll<TypeAlignment>(in, out);
    }

    while (in < sentinel)
        _internals::snippetCumsum(in, out);
}

template <>
void sfz::sfzInterpolationCast<float, true>(absl::Span<const float> floatJumps, absl::Span<int> jumps, absl::Span<float> leftCoeffs, absl::Span<float> rightCoeffs) noexcept
{
    sfz::sfzInterpolationCast<float, false>(floatJumps, jumps, leftCoeffs, rightCoeffs);
    // CHECK(jumps.size() >= floatJumps.size());
    // CHECK(jumps.size() == leftCoeffs.size());
    // CHECK(jumps.size() == rightCoeffs.size());

    // auto floatJump = floatJumps.data();
    // auto jump = jumps.data();
    // auto leftCoeff = leftCoeffs.data();
    // auto rightCoeff = rightCoeffs.data();
    // const auto sentinel = floatJump + min(floatJumps.size(), jumps.size(), leftCoeffs.size(), rightCoeffs.size());
    // const auto lastAligned = prevAligned(sentinel);

    // while (unaligned(floatJump, reinterpret_cast<float*>(jump), leftCoeff, rightCoeff) && floatJump < lastAligned)
    //     _internals::snippetSFZInterpolationCast(floatJump, jump, leftCoeff, rightCoeff);

    // while (floatJump < lastAligned) {
    //     auto mmFloatJumps = _mm_load_ps(floatJump);
    //     auto mmIndices = _mm_cvtps_epi32(_mm_sub_ps(mmFloatJumps, _mm_set_ps1(0.4999999552965164184570312f)));
    //     _mm_store_si128(reinterpret_cast<__m128i*>(jump), mmIndices);

    //     auto mmRight = _mm_sub_ps(mmFloatJumps, _mm_cvtepi32_ps(mmIndices));
    //     auto mmLeft = _mm_sub_ps(_mm_set_ps1(1.0f), mmRight);
    //     _mm_store_ps(leftCoeff, mmLeft);
    //     _mm_store_ps(rightCoeff, mmRight);
    //     incrementAll<TypeAlignment>(floatJump, jump, leftCoeff, rightCoeff);
    // }

    // while(floatJump < sentinel)
    //     _internals::snippetSFZInterpolationCast(floatJump, jump, leftCoeff, rightCoeff);
}

template <>
void sfz::diff<float, true>(absl::Span<const float> input, absl::Span<float> output) noexcept
{
    CHECK(output.size() >= input.size());
    if (input.size() == 0)
        return;

    auto out = output.data();
    auto in = input.data();
    const auto sentinel = in + std::min(input.size(), output.size());
    const auto lastAligned = prevAligned(sentinel);

    *out++ = *in++;
    while (unaligned(in, out) && in < lastAligned)
        _internals::snippetDiff(in, out);

    auto mmBase = _mm_set_ps1(*(in - 1));
    while (in < lastAligned) {
        auto mmOutput = _mm_load_ps(in);
        auto mmNextBase = _mm_shuffle_ps(mmOutput, mmOutput, _MM_SHUFFLE(3, 3, 3, 3));
        mmOutput = _mm_sub_ps(mmOutput, mmBase);
        mmBase = mmNextBase;
        mmOutput = _mm_sub_ps(mmOutput, _mm_castsi128_ps(_mm_slli_si128(_mm_castps_si128(mmOutput), 4)));
        _mm_store_ps(out, mmOutput);
        incrementAll<TypeAlignment>(in, out);
    }

    while (in < sentinel)
        _internals::snippetDiff(in, out);
}

#endif // SFIZZ_HAVE_SSE2
