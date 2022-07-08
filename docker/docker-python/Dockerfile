#
# Experimental dockerfile for link-grammar python bindings.
#
# XXX TODO: actually run the python unit tests!
#
FROM linkgrammar/lgbase:latest
MAINTAINER Linas Vepstas linasvepstas@gmail.com

RUN apt-get update && apt-get install git build-essential python3-dev libpcre2-dev python-is-python3 libtre-dev wget automake locales libtool flex m4 autoconf-archive autoconf pkg-config swig libthai-dev help2man -y && rm -rf /var/lib/apt/lists/*

# Perform the standard build.
RUN (cd link-grammar-5*; mkdir build; cd build; ../configure --disable-java-bindings --enable-python-bindings; make -j12; make install; ldconfig)

RUN adduser --disabled-password --gecos "Link Parser User" link-parser

USER link-parser
WORKDIR /home/link-parser
RUN echo "export LANG=en_US.UTF-8" >> .bash_aliases
CMD bash

RUN export LANG=en_US.UTF-8

RUN echo "this is a test" | link-parser

