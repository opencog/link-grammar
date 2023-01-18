#!/bin/bash
set -ex

pushd "$(dirname "$0")"

# Activate emscripten sdk if not done already
[ ! $EMSDK ] && pushd emsdk && source ./emsdk_env.sh && popd

# Compile LLVM bitcode
# XXX FIXME: We'd like to enable pcre2, but somehow,
# this fails in the github automated testing environment.
pushd ../..
emconfigure ./configure --disable-editline --disable-sat-solver --disable-java-bindings --disable-python-bindings --disable-pcre2
emmake make clean
emmake make
popd

# Build link-grammar
emcc -O3 link-grammar/.libs/liblink-grammar.a -o liblink-grammar.js

# Build link-parser
./link-parser/build.sh

popd
