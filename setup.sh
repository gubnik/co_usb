#!/usr/bin/sh
PRESET=$1
if [ -z $PRESET ]; then
    echo Preset missing
    exit 127
fi
BUILD_TYPE=$2
if [ -z $BUILD_TYPE ]; then
    echo Build type missing
    exit 127
fi
if [ -z $VCPKG_ROOT ]; then
    echo No vcpkg found. Cease!!
    exit 127
fi
echo "\033[1;33mvcpkg located at $VCPKG_ROOT\033[0;0m"
rm -rf build/$PRESET
mkdir -p build/$PRESET
cmake --preset=$PRESET && \
    ln -s build/$PRESET/compile_commands.json compile_commands.json && \
    cmake --build build/$PRESET --config ${BUILD_TYPE}
