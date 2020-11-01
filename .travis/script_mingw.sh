#!/bin/bash

set -ex

mkdir -p build/${INSTALL_DIR} && cd build
if [[ ${CROSS_COMPILE} == "mingw32" ]]; then
  cmake -DCMAKE_TOOLCHAIN_FILE=../.travis/toolchain-i686-w64-mingw32.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DSFIZZ_USE_SNDFILE=OFF \
        -DENABLE_LTO=OFF \
        -DSFIZZ_JACK=OFF \
        -DSFIZZ_VST=ON \
        -DSFIZZ_RENDER=OFF \
        -DSFIZZ_STATIC_DEPENDENCIES=ON \
        -DCMAKE_CXX_STANDARD=17 \
        ..
  make -j2
elif [[ ${CROSS_COMPILE} == "mingw64" ]]; then
  cmake  -DCMAKE_TOOLCHAIN_FILE=../.travis/toolchain-x86_64-w64-mingw32.cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DSFIZZ_USE_SNDFILE=OFF \
        -DENABLE_LTO=OFF \
        -DSFIZZ_JACK=OFF \
        -DSFIZZ_VST=ON \
        -DSFIZZ_RENDER=OFF \
        -DSFIZZ_STATIC_DEPENDENCIES=ON \
        -DCMAKE_CXX_STANDARD=17 \
        ..
  make -j2
fi
