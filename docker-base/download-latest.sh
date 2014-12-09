#! /bin/bash
#
# Download the current released version of link-grammar.
# The wget tries to guess the correct file to download.
#
wget -r --no-parent -nH --cut-dirs=2 http://www.abisource.com/downloads/link-grammar/current/

# Unpack the sources, too.
tar -zxf current/link-grammar-5*.tar.gz
