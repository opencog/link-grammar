#!/bin/bash
set -ex

pushd "$(dirname "$0")"

# Activate emscripten sdk if not done already
[ ! $EMSDK ] && pushd emsdk && source ./emsdk_env.sh && popd

# Compile LLVM bitcode
pushd ..
emconfigure ./configure --disable-editline --disable-sat-solver --disable-java-bindings --disable-python-bindings
emmake make
popd

# Build link-parser
./link-parser/build.sh

# TODO: Build link-grammar
# emcc -O3 link-grammar/.libs/liblink-grammar.so -o liblink-grammar.js

popd
