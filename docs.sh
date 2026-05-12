#!/usr/bin/sh

ROOT=$(pwd)
cmake --preset x64-static -DCOUSB_ONLY_BUILD_DOCS=ON && \
cmake --build build/x64-static --target docs && \
mkdir -p docs/html && \
cp $ROOT/build/x64-static/docs/html $ROOT/docs/html -r && \
printf "\033[1;33mFinished building docs\033[0;0m\n"
