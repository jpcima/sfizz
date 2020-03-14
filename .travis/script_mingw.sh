#!/bin/bash

set -ex
. .travis/docker_container.sh

mkdir -p build/${INSTALL_DIR} && cd build
if [[ ${CROSS_COMPILE} == "mingw32" ]]; then
  buildenv i686-w64-mingw32-cmake -DCMAKE_BUILD_TYPE=Release \
                                  -DENABLE_LTO=OFF \
                                  -DSFIZZ_JACK=OFF \
                                  -DSFIZZ_VST=ON \
                                  -DSFIZZ_STATIC_LIBSNDFILE=ON \
                                  -DCMAKE_CXX_STANDARD=17 \
                                  ..
  buildenv patch -p1 -i "$(pwd)"/../.travis/patches/vstsdk-3.6.14-mingw-atomic-add.diff -d ../vst/external/VST_SDK
  buildenv make -j$(nproc)
elif [[ ${CROSS_COMPILE} == "mingw64" ]]; then
  buildenv x86_64-w64-mingw32-cmake -DCMAKE_BUILD_TYPE=Release \
                                    -DENABLE_LTO=OFF \
                                    -DSFIZZ_JACK=OFF \
                                    -DSFIZZ_VST=ON \
                                    -DSFIZZ_STATIC_LIBSNDFILE=ON \
                                    -DCMAKE_CXX_STANDARD=17 \
                                    ..
  buildenv patch -p1 -i "$(pwd)"/../.travis/patches/vstsdk-3.6.14-mingw-atomic-add.diff -d ../vst/external/VST_SDK
  buildenv make -j$(nproc)
fi
