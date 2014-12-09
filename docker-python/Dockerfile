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

RUN apt-get install -y swig2.0

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

