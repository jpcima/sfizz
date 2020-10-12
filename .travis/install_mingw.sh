#!/bin/bash

set -ex
. .travis/msys2_env.sh

#$msys2 pacman --sync --quiet --refresh --sysupgrade --noconfirm

$msys2 pacman --sync --quiet --noconfirm --needed \
       mingw-w64-"$CI_ARCH"-toolchain \
       mingw-w64-"$CI_ARCH"-cmake \
       mingw-w64-"$CI_ARCH"-libsndfile

## Install more MSYS2 packages from https://packages.msys2.org/base here
taskkill //IM gpg-agent.exe //F || true # https://travis-ci.community/t/4967

$mingw gcc -v && $mingw g++ -v && $mingw cmake --version
