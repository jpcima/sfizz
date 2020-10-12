#!/bin/bash

set -ex
. .travis/msys2_env.sh

mkdir -p build/${INSTALL_DIR} && cd build
$mingw cmake -G "MinGW Makefiles" \
               -DCMAKE_BUILD_TYPE=Release \
               -DENABLE_LTO=OFF \
               -DSFIZZ_JACK=OFF \
               -DSFIZZ_VST=ON \
               -DSFIZZ_STATIC_DEPENDENCIES=ON \
               -DCMAKE_CXX_STANDARD=17 \
               ..
$mingw cmake --build . -j "$(nproc)"
