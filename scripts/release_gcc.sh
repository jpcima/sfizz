#!/bin/sh
script_dir="$(dirname "$0")"
cmake -D CMAKE_BUILD_TYPE=Release -S "$script_dir/.." -B .
