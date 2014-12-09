#! /bin/bash
#
# Download and build teh current released version of link-grammar
# The wget tries to guess the correct file to download.
#
# Perform the standard build.
cd link-grammar-5*
mkdir build
cd build
export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
../configure
make -j12
make install
ldconfig
