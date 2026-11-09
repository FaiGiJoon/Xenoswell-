#!/bin/sh
# Builds build/xenoswell.cue and .bin inside the toolchain image.
set -e
cd "$(dirname "$0")/.."
docker build -q --platform linux/amd64 -t xenoswell-sdk . >/dev/null
docker run --rm --platform linux/amd64 -v "$PWD":/src xenoswell-sdk sh -c \
 'cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=/opt/psn00b/lib/libpsn00b/cmake/sdk.cmake -DPSN00BSDK_TC=/opt/gcc-root && cmake --build build'
ls -l build/xenoswell.cue build/xenoswell.bin
