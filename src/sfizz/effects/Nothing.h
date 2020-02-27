// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Effects.h"

namespace sfz {
namespace fx {

/**
   @brief Effect which does nothing
 */
class Nothing : public Effect {
public:
    /**
       @brief Initializes with the given sample rate.
     */
    void init(double sampleRate) override;

    /**
       @brief Reset the state to initial.
     */
    void clear() override;

    /**
       @brief Copy the input signal to the output
    */
    void process(const float* inputs[], float* outputs[], unsigned nframes) override;
};

} // namespace fx
} // namespace sfz
