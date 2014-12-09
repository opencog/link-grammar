#! /bin/bash
#
# Download and build teh current released version of link-grammar
# The wget tries to guess the correct file to download.
#

# Perform the standard build.
cd link-grammar-5*
mkdir build
cd build
../configure --disable-java-bindings --enable-python-bindings
make -j12
make install
ldconfig
