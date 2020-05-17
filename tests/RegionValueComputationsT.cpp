// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz/Defaults.h"
#include "sfizz/Region.h"
#include "sfizz/SfzHelpers.h"
#include "sfizz/MidiState.h"
#include "catch2/catch.hpp"
#include <chrono>
#include <thread>
using namespace Catch::literals;
using namespace sfz::literals;
constexpr int numRandomTests { 64 };
TEST_CASE("[Region] Crossfade in on key")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lokey", "1" });
    region.parseOpcode({ "xfin_hikey", "3" });
    REQUIRE(region.getNoteGain(2, 127_norm) == 0.70711_a);
    REQUIRE(region.getNoteGain(1, 127_norm) == 0.0_a);
    REQUIRE(region.getNoteGain(3, 127_norm) == 1.0_a);
}

TEST_CASE("[Region] Crossfade in on key - 2")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lokey", "1" });
    region.parseOpcode({ "xfin_hikey", "5" });
    REQUIRE(region.getNoteGain(1, 127_norm) == 0.0_a);
    REQUIRE(region.getNoteGain(2, 127_norm) == 0.5_a);
    REQUIRE(region.getNoteGain(3, 127_norm) == 0.70711_a);
    REQUIRE(region.getNoteGain(4, 127_norm) == 0.86603_a);
    REQUIRE(region.getNoteGain(5, 127_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(6, 127_norm) == 1.0_a);
}

TEST_CASE("[Region] Crossfade in on key - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lokey", "1" });
    region.parseOpcode({ "xfin_hikey", "5" });
    region.parseOpcode({ "xf_keycurve", "gain" });
    REQUIRE(region.getNoteGain(1, 127_norm) == 0.0_a);
    REQUIRE(region.getNoteGain(2, 127_norm) == 0.25_a);
    REQUIRE(region.getNoteGain(3, 127_norm) == 0.5_a);
    REQUIRE(region.getNoteGain(4, 127_norm) == 0.75_a);
    REQUIRE(region.getNoteGain(5, 127_norm) == 1.0_a);
}

TEST_CASE("[Region] Crossfade out on key")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lokey", "51" });
    region.parseOpcode({ "xfout_hikey", "55" });
    REQUIRE(region.getNoteGain(50, 127_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(51, 127_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(52, 127_norm) == 0.86603_a);
    REQUIRE(region.getNoteGain(53, 127_norm) == 0.70711_a);
    REQUIRE(region.getNoteGain(54, 127_norm) == 0.5_a);
    REQUIRE(region.getNoteGain(55, 127_norm) == 0.0_a);
    REQUIRE(region.getNoteGain(56, 127_norm) == 0.0_a);
}

TEST_CASE("[Region] Crossfade out on key - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lokey", "51" });
    region.parseOpcode({ "xfout_hikey", "55" });
    region.parseOpcode({ "xf_keycurve", "gain" });
    REQUIRE(region.getNoteGain(50, 127_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(51, 127_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(52, 127_norm) == 0.75_a);
    REQUIRE(region.getNoteGain(53, 127_norm) == 0.5_a);
    REQUIRE(region.getNoteGain(54, 127_norm) == 0.25_a);
    REQUIRE(region.getNoteGain(55, 127_norm) == 0.0_a);
    REQUIRE(region.getNoteGain(56, 127_norm) == 0.0_a);
}

TEST_CASE("[Region] Crossfade in on velocity")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lovel", "20" });
    region.parseOpcode({ "xfin_hivel", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE(region.getNoteGain(1, 19_norm) == 0.0_a);
    REQUIRE(region.getNoteGain(1, 20_norm) == 0.0_a);
    REQUIRE(region.getNoteGain(2, 21_norm) == 0.5_a);
    REQUIRE(region.getNoteGain(3, 22_norm) == 0.70711_a);
    REQUIRE(region.getNoteGain(4, 23_norm) == 0.86603_a);
    REQUIRE(region.getNoteGain(5, 24_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(6, 25_norm) == 1.0_a);
}

TEST_CASE("[Region] Crossfade in on vel - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_lovel", "20" });
    region.parseOpcode({ "xfin_hivel", "24" });
    region.parseOpcode({ "xf_velcurve", "gain" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE(region.getNoteGain(1, 19_norm) == 0.0_a);
    REQUIRE(region.getNoteGain(1, 20_norm) == 0.0_a);
    REQUIRE(region.getNoteGain(2, 21_norm) == 0.25_a);
    REQUIRE(region.getNoteGain(3, 22_norm) == 0.5_a);
    REQUIRE(region.getNoteGain(4, 23_norm) == 0.75_a);
    REQUIRE(region.getNoteGain(5, 24_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(5, 25_norm) == 1.0_a);
}

TEST_CASE("[Region] Crossfade out on vel")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lovel", "51" });
    region.parseOpcode({ "xfout_hivel", "55" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE(region.getNoteGain(5, 50_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(5, 51_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(5, 52_norm) == 0.86603_a);
    REQUIRE(region.getNoteGain(5, 53_norm) == 0.70711_a);
    REQUIRE(region.getNoteGain(5, 54_norm) == 0.5_a);
    REQUIRE(region.getNoteGain(5, 55_norm) == 0.0_a);
    REQUIRE(region.getNoteGain(5, 56_norm) == 0.0_a);
}

TEST_CASE("[Region] Crossfade out on vel - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_lovel", "51" });
    region.parseOpcode({ "xfout_hivel", "55" });
    region.parseOpcode({ "xf_velcurve", "gain" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE(region.getNoteGain(56, 50_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(56, 51_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(56, 52_norm) == 0.75_a);
    REQUIRE(region.getNoteGain(56, 53_norm) == 0.5_a);
    REQUIRE(region.getNoteGain(56, 54_norm) == 0.25_a);
    REQUIRE(region.getNoteGain(56, 55_norm) == 0.0_a);
    REQUIRE(region.getNoteGain(56, 56_norm) == 0.0_a);
}

TEST_CASE("[Region] Crossfade in on CC")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_locc24", "20" });
    region.parseOpcode({ "xfin_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    midiState.ccEvent(0, 24, 19_norm);
    REQUIRE(region.getCrossfadeGain() == 0.0_a);
    midiState.ccEvent(0, 24, 20_norm);
    REQUIRE(region.getCrossfadeGain() == 0.0_a);
    midiState.ccEvent(0, 24, 21_norm);
    REQUIRE(region.getCrossfadeGain() == 0.5_a);
    midiState.ccEvent(0, 24, 22_norm);
    REQUIRE(region.getCrossfadeGain() == 0.70711_a);
    midiState.ccEvent(0, 24, 23_norm);
    REQUIRE(region.getCrossfadeGain() == 0.86603_a);
    midiState.ccEvent(0, 24, 24_norm);
    REQUIRE(region.getCrossfadeGain() == 1.0_a);
    midiState.ccEvent(0, 24, 25_norm);
    REQUIRE(region.getCrossfadeGain() == 1.0_a);
}

TEST_CASE("[Region] Crossfade in on CC - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfin_locc24", "20" });
    region.parseOpcode({ "xfin_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    region.parseOpcode({ "xf_cccurve", "gain" });
    midiState.ccEvent(0, 24, 19_norm);
    REQUIRE(region.getCrossfadeGain() == 0.0_a);
    midiState.ccEvent(0, 24, 20_norm);
    REQUIRE(region.getCrossfadeGain() == 0.0_a);
    midiState.ccEvent(0, 24, 21_norm);
    REQUIRE(region.getCrossfadeGain() == 0.25_a);
    midiState.ccEvent(0, 24, 22_norm);
    REQUIRE(region.getCrossfadeGain() == 0.5_a);
    midiState.ccEvent(0, 24, 23_norm);
    REQUIRE(region.getCrossfadeGain() == 0.75_a);
    midiState.ccEvent(0, 24, 24_norm);
    REQUIRE(region.getCrossfadeGain() == 1.0_a);
    midiState.ccEvent(0, 24, 25_norm);
    REQUIRE(region.getCrossfadeGain() == 1.0_a);
}
TEST_CASE("[Region] Crossfade out on CC")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_locc24", "20" });
    region.parseOpcode({ "xfout_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    midiState.ccEvent(0, 24, 19_norm);
    REQUIRE(region.getCrossfadeGain() == 1.0_a);
    midiState.ccEvent(0, 24, 20_norm);
    REQUIRE(region.getCrossfadeGain() == 1.0_a);
    midiState.ccEvent(0, 24, 21_norm);
    REQUIRE(region.getCrossfadeGain() == 0.86603_a);
    midiState.ccEvent(0, 24, 22_norm);
    REQUIRE(region.getCrossfadeGain() == 0.70711_a);
    midiState.ccEvent(0, 24, 23_norm);
    REQUIRE(region.getCrossfadeGain() == 0.5_a);
    midiState.ccEvent(0, 24, 24_norm);
    REQUIRE(region.getCrossfadeGain() == 0.0_a);
    midiState.ccEvent(0, 24, 25_norm);
    REQUIRE(region.getCrossfadeGain() == 0.0_a);
}

TEST_CASE("[Region] Crossfade out on CC - gain")
{
	sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "xfout_locc24", "20" });
    region.parseOpcode({ "xfout_hicc24", "24" });
    region.parseOpcode({ "amp_veltrack", "0" });
    region.parseOpcode({ "xf_cccurve", "gain" });
    midiState.ccEvent(0, 24, 19_norm);
    REQUIRE(region.getCrossfadeGain() == 1.0_a);
    midiState.ccEvent(0, 24, 20_norm);
    REQUIRE(region.getCrossfadeGain() == 1.0_a);
    midiState.ccEvent(0, 24, 21_norm);
    REQUIRE(region.getCrossfadeGain() == 0.75_a);
    midiState.ccEvent(0, 24, 22_norm);
    REQUIRE(region.getCrossfadeGain() == 0.5_a);
    midiState.ccEvent(0, 24, 23_norm);
    REQUIRE(region.getCrossfadeGain() == 0.25_a);
    midiState.ccEvent(0, 24, 24_norm);
    REQUIRE(region.getCrossfadeGain() == 0.0_a);
    midiState.ccEvent(0, 24, 25_norm);
    REQUIRE(region.getCrossfadeGain() == 0.0_a);
}

TEST_CASE("[Region] Velocity bug for extreme values - veltrack at 0")
{
    sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "amp_veltrack", "0" });
    REQUIRE(region.getNoteGain(64, 127_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(64, 0_norm) == 1.0_a);
}


TEST_CASE("[Region] Velocity bug for extreme values - positive veltrack")
{
    sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "amp_veltrack", "100" });
    REQUIRE(region.getNoteGain(64, 127_norm) == 1.0_a);
    REQUIRE(region.getNoteGain(64, 0_norm) == Approx(0.0).margin(0.0001));
}

TEST_CASE("[Region] Velocity bug for extreme values - negative veltrack")
{
    sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "amp_veltrack", "-100" });
    REQUIRE(region.getNoteGain(64, 127_norm) == Approx(0.0).margin(0.0001));
    REQUIRE(region.getNoteGain(64, 0_norm) == 1.0_a);
}

TEST_CASE("[Region] rt_decay")
{
    sfz::MidiState midiState;
    midiState.setSampleRate(1000);
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "trigger", "release" });
    region.parseOpcode({ "rt_decay", "10" });
    midiState.noteOnEvent(0, 64, 64_norm);
    midiState.advanceTime(100);
    REQUIRE( region.getBaseVolumedB(64) == Approx(sfz::Default::volume - 1.0f).margin(0.1) );
    region.parseOpcode({ "rt_decay", "20" });
    midiState.noteOnEvent(0, 64, 64_norm);
    midiState.advanceTime(100);
    REQUIRE( region.getBaseVolumedB(64) == Approx(sfz::Default::volume - 2.0f).margin(0.1) );
    region.parseOpcode({ "trigger", "attack" });
    midiState.noteOnEvent(0, 64, 64_norm);
    midiState.advanceTime(100);
    REQUIRE( region.getBaseVolumedB(64) == Approx(sfz::Default::volume).margin(0.1) );
}

TEST_CASE("[Region] Base delay")
{
    sfz::MidiState midiState;
    sfz::Region region { 0, midiState };
    region.parseOpcode({ "sample", "*sine" });
    region.parseOpcode({ "delay", "10" });
    REQUIRE( region.getDelay() == 10.0f );
    region.parseOpcode({ "delay_random", "10" });
    Random::randomGenerator.seed(42);
    for (int i = 0; i < numRandomTests; ++i)
    {
        auto delay = region.getDelay();
        REQUIRE( (delay >= 10.0 && delay <= 20.0) );
    }
}
