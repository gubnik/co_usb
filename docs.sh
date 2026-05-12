#!/usr/bin/sh

ROOT=$(pwd)
PRESET=no-vcpkg
cmake --preset $PRESET -DCOUSB_ONLY_BUILD_DOCS=ON && \
cmake --build build/$PRESET --target docs && \
mkdir -p docs/html && \
cp $ROOT/build/$PRESET/docs/html/* $ROOT/docs/html -r && \
printf "\033[1;33mFinished building docs\033[0;0m\n"
