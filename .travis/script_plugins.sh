#!/bin/bash
set -ex

mkdir -p build/${INSTALL_DIR} && cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DSFIZZ_JACK=OFF \
      -DSFIZZ_VST="$ENABLE_VST_PLUGIN" \
      -DSFIZZ_TESTS=OFF \
      -DSFIZZ_SHARED=OFF \
      -DSFIZZ_STATIC_LIBSNDFILE=ON \
      -DCMAKE_CXX_STANDARD=17 \
      ..
make -j$(nproc)
