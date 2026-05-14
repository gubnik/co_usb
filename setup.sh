#!/usr/bin/sh
PRESET=$1
if [ -z "$PRESET" ]; then
    printf 'Preset missing\n' >&2
    exit 127
fi

BUILD_TYPE=$2
if [ -z "$BUILD_TYPE" ]; then
    printf 'Build type missing\n' >&2
    exit 127
fi

if [ -z "$VCPKG_ROOT" ]; then
    printf 'No vcpkg found. Cease!!\n' >&2
    exit 127
fi

printf '\033[1;33mvcpkg located at %s\033[0;0m\n' "$VCPKG_ROOT"

if [ "$3" = "reset" ]; then
    rm -rf "build/$PRESET"
    mkdir -p "build/$PRESET"
fi

cmake --preset="$PRESET" -DCOUSB_BUILD_DOCS=OFF && \
cmake --build "build/$PRESET" --config "$BUILD_TYPE"

rm -f compile_commands.json && \
ln -s "build/$PRESET/compile_commands.json" compile_commands.json 2>/dev/null || :
