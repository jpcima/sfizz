#!/bin/bash
set -ex

mkdir -p build/${INSTALL_DIR} && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSFIZZ_USE_SNDFILE=OFF \
      -DSFIZZ_JACK=OFF \
      -DSFIZZ_VST="$ENABLE_VST_PLUGIN" \
      -DSFIZZ_LV2_UI="$ENABLE_LV2_UI" \
      -DSFIZZ_TESTS=OFF \
      -DSFIZZ_SHARED=OFF \
      -DSFIZZ_STATIC_DEPENDENCIES=ON \
      -DSFIZZ_RENDER=OFF \
      -DCMAKE_CXX_STANDARD=17 \
      ..
make -j2
