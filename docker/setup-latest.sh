#! /bin/bash
#
wget -r --no-parent -nH --cut-dirs=2 http://www.abisource.com/downloads/link-grammar/current/

tar -xvf current/link-grammar-5*.tar.gz
cd link-grammar-5*
mkdir build
cd build
../configure
make -j12
make install
ldconfig
