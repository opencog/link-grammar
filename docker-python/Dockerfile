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
FROM gcc:latest
MAINTAINER linasvepstas@gmail.com

RUN apt-get update
RUN apt-get -y autoremove
RUN apt-get -y install apt-utils

# The parse server needs jni.h -- this is furnished by jdk
# which in turn has a HUGE dependency list :-(
RUN apt-get install -y openjdk-7-jdk
RUN apt-get install -y ant

# Need wget to download link-grammar source
RUN apt-get install wget

# Need the locales for utf8
RUN apt-get install locales
RUN echo "en_US.UTF-8 UTF-8" > /etc/locale.gen
RUN echo "ru_RU.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "he_IL.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "de_DE.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "lt_LT.UTF-8 UTF-8" >> /etc/locale.gen
RUN echo "tr_TR.UTF-8 UTF-8" >> /etc/locale.gen
RUN locale-gen 

# Now, download and build.
ADD setup-latest.sh /
RUN chmod +x setup-latest.sh
RUN /setup-latest.sh

RUN adduser --disabled-password --gecos "Link Parser User" link-parser

USER link-parser
WORKDIR /home/link-parser
RUN echo "export LANG=en_US.UTF-8" >> .bash_aliases
CMD bash

RUN export LANG=en_US.UTF-8

RUN echo "this is a test" | link-parser
# 

