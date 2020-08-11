#!/bin/bash
set -ex

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSFIZZ_JACK=OFF \
      -DSFIZZ_TESTS=ON \
      -DSFIZZ_SHARED=OFF \
      -DSFIZZ_STATIC_LIBSNDFILE=OFF \
      -DSFIZZ_LV2=OFF \
      -DCMAKE_CXX_STANDARD=17 \
      ..
make -j$(nproc) sfizz_tests
tests/sfizz_tests

make -j$(nproc) sfizz_plot_lfo
tests/sfizz_plot_lfo tests/lfo/lfo_waves.sfz 100 > lfo_waves.dat
tests/sfizz_plot_lfo tests/lfo/lfo_subwave.sfz 100 > lfo_subwave.dat
tests/sfizz_plot_lfo tests/lfo/lfo_fade_and_delay.sfz 100 > lfo_fade_and_delay.dat

tests/lfo/compare_lfo.py lfo_waves.dat tests/lfo/lfo_waves_reference.dat
tests/lfo/compare_lfo.py lfo_subwave.dat tests/lfo/lfo_subwave_reference.dat
tests/lfo/compare_lfo.py lfo_fade_and_delay.dat tests/lfo/lfo_fade_and_delay_reference.dat
