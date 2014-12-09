#! /bin/bash
#
# Build the current released version of the link-grammar parser
#
# Perform the standard build.
# Assumes that the sources have already been unpacked.
cd link-grammar-5*
mkdir build
cd build
../configure --disable-java-bindings
make -j12
make install
ldconfig
