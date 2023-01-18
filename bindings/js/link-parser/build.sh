#!/bin/bash
set -ex

pushd "$(dirname "$0")"

# Activate emscripten sdk if not done already
[ ! $EMSDK ] && pushd ../emsdk && source ./emsdk_env.sh && popd

# Create empty `dist` folder
rm -rf dist
mkdir dist

# Compile link-parser.js & link-parser.wasm
# XXX Do we really need to do this?? It seems that a link-parser.wasm
# file was already created in the ../../../link-parser/.libs directory;
# can't we just use that? Someone who knows jas & wasm needs to fix this...
cp ../../../link-parser/link_parser-command-line.o command-line.bc
cp ../../../link-parser/link_parser-lg_readline.o lg_readline.bc
cp ../../../link-parser/link_parser-parser-utilities.o parser-utilities.bc
cp ../../../link-parser/link_parser-link-parser.o link-parser.bc
# Originally, this had `emcc liblink-grammar.so` but the .so
# is not available on Apple Mac's (I guess it's called .shlib ??)
# So change to .a which should work for all OS'es.
emcc -O3 link-parser.bc command-line.bc lg_readline.bc parser-utilities.bc \
	../../../link-grammar/.libs/liblink-grammar.a \
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
