#!/bin/bash
mkdir -p build
pushd build > /dev/null

COMMON_FLAGS=(-Werror -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wno-unused-function -Wno-deprecated-declarations -Wno-unused-variable -Wno-unused-parameter -Wno-c11-extensions)

clang -std=c99 -g -O0 -dynamiclib "${COMMON_FLAGS[@]}" $(pkg-config --cflags sdl3) ../code/game.c -o game.dylib $(pkg-config --libs sdl3)

clang -std=c99 -g -O0 "${COMMON_FLAGS[@]}" $(pkg-config --cflags sdl3 vulkan) ../code/vulcan.c -o Vulcan $(pkg-config --libs sdl3 vulkan)

popd > /dev/null