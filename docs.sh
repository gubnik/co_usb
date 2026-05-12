#!/usr/bin/sh

ROOT=$(pwd)
cmake --preset x64-static && \
cd build/x64-static && \
ninja docs && \
mkdir -p docs/html && \
cp $ROOT/build/x64-static/docs/html $ROOT/docs/html -r && \
printf "\033[1;33mFinished building docs\033[0;0m\n"
