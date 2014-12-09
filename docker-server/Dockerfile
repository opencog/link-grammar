#
# Experimental dockerfile for link-grammar.
#
# The problem is that the Debian/Ubuntu/Mint link-grammar.debs
# are old & stale. So instead, we compile from scratch.
#
# We'll need two different use-cases:
# 1) the command-line client
# 2) the parse server
#
# Anyway...
#
FROM linkgrammar/lgbase:latest
MAINTAINER linasvepstas@gmail.com

# The parse server needs jni.h -- this is furnished by jdk
# which in turn has a HUGE dependency list :-(
# This is the reason we are doing multiple docker files, and not one.
RUN apt-get install -y openjdk-7-jdk
RUN apt-get install -y ant

# Perform the standard build.
ENV JAVA_HOME /usr/lib/jvm/java-7-openjdk-amd64
RUN (cd link-grammar-5*; mkdir build; cd build; ../configure; make -j12; make install; ldconfig)

RUN adduser --disabled-password --gecos "Link Parser User" link-parser

USER link-parser
WORKDIR /home/link-parser
RUN echo "export LANG=en_US.UTF-8" >> .bash_aliases
CMD bash

RUN export LANG=en_US.UTF-8

RUN echo "this is a test" | link-parser
# 

