#!/bin/bash
set -ex

pushd "$(dirname "$0")"

# Activate emscripten sdk if not done already
[ ! $EMSDK ] && pushd ../emsdk && source ./emsdk_env.sh && popd

# Create empty `dist` folder
rm -rf dist
mkdir dist

# Compile link-parser.js & link-parser.wasm
cp ../../../link-parser/.libs/link-parser link-parser.bc
# Originally, this had `emcc liblink-grammar.so` but the .so
# is not available on Apple Mac's (I guess it's called .shlib ??)
# So change to .a which should work for all OS'es.
emcc -O3 link-parser.bc ../../../link-grammar/.libs/liblink-grammar.a \
	libpthread.a \
	--pre-js pre.js \
	-s WASM=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
    -o dist/link-parser.js
rm link-parser.bc

# Copy contents of `data` dir
cp -r ../../../data dist/data

# Copy needed contents of this dir
cp bin.js package.json README.md dist

# Test
pushd dist && npm link && popd
echo The needs of the many outweigh the needs of the few. | link-parser
pushd dist && npm unlink && popd

popd
