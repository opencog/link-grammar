#! /bin/bash
#
# Download and build teh current released version of link-grammar
# The wget tries to guess the correct file to download.
#
wget -r --no-parent -nH --cut-dirs=2 http://www.abisource.com/downloads/link-grammar/current/

# Perform the standard build.
tar -xvf current/link-grammar-5*.tar.gz
cd link-grammar-5*
mkdir build
cd build
# ../configure --enable-python-bindings
export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
../configure
make -j12
make install
ldconfig
