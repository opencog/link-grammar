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

RUN apt-get install -y swig2.0
RUN apt-get install -y python
RUN apt-get install -y python-dev

# Now, download and build.
ADD build-python.sh /
RUN chmod +x build-python.sh
RUN /build-python.sh

RUN adduser --disabled-password --gecos "Link Parser User" link-parser

USER link-parser
WORKDIR /home/link-parser
RUN echo "export LANG=en_US.UTF-8" >> .bash_aliases
CMD bash

RUN export LANG=en_US.UTF-8

RUN echo "this is a test" | link-parser

