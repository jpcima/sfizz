// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "sfizz.hpp"
#include "sfizz/Config.h"
#include "sfizz/utility/BitArray.h"
#include <string>
#include <cstdint>

// Note: these functions are not RT

BitArray<sfz::config::numCCs> queryCCSlots(sfz::Sfizz& synth, int delay = 0);
float queryCCDefaultValue(sfz::Sfizz& synth, uint32_t cc, int delay = 0);
std::string queryCCLabel(sfz::Sfizz& synth, uint32_t cc, int delay = 0);
