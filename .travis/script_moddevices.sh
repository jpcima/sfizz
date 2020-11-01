#!/bin/bash

set -ex
. .travis/docker_container.sh

mkdir -p build/${INSTALL_DIR} && cd build

buildenv mod-plugin-builder /usr/local/bin/cmake \
         -DSFIZZ_SYSTEM_PROCESSOR=armv7-a \
         -DCMAKE_BUILD_TYPE=Release \
         -DSFIZZ_USE_SNDFILE=OFF \
         -DSFIZZ_JACK=OFF \
         -DSFIZZ_LV2_UI=OFF ..
buildenv mod-plugin-builder make -j2
