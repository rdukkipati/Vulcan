#!/bin/bash
mkdir -p build
pushd build > /dev/null


COMMON_FLAGS=(-Werror -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wno-unused-function -Wno-deprecated-declarations -Wno-c11-extensions -Wno-unused-parameter -Wno-unused-variable)


clang -std=c99 -g -O0 -DOS_LINUX=1 -DDEBUG=1 -shared -fPIC "${COMMON_FLAGS[@]}" $(pkg-config --cflags sdl3) ../code/game.c -o game.so $(pkg-config --libs sdl3)

clang -std=c99 -g -O0 -DOS_LINUX=1 -DDEBUG=1 "${COMMON_FLAGS[@]}" $(pkg-config --cflags sdl3 vulkan) ../code/vulcan.c -o Vulcan $(pkg-config --libs sdl3 vulkan)

popd > /dev/null