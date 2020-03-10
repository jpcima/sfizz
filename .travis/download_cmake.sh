#!/bin/bash

set -ex

cmake_os_name="$1"
cmake_cpu_arch="$2"

if [ -z "$1" ]; then
  cmake_os_name="${TRAVIS_OS_NAME}"
fi
if [ -z "$2" ]; then
  cmake_cpu_arch="${TRAVIS_CPU_ARCH}"
fi

cmake_dir="cmake-3.13.0-${cmake_os_name}-${cmake_cpu_arch}"
cmake_arc="${cmake_dir}.tar.gz"
cmake_url="https://github.com/sfztools/cmake/releases/download/${cmake_os_name}/${cmake_arc}"

wget -q "${cmake_url}"
tar xzf "${cmake_arc}"
pushd "${cmake_dir}"
sudo cp    bin/*   /usr/local/bin/
sudo cp -r share/* /usr/local/share/
popd
