#
# Experimental dockerfile for link-grammar.
# The problem is that the Debian/Ubuntu/Mint link-grammar.debs
# are old & stale. So instead, we compile from scratch.
#
# We'll need two different use-cases:
# 1) the command-line client
# 2) the parse server
#

FROM gcc:latest
MAINTAINER linasvepstas@gmail.com

ADD setup-latest.sh /
RUN chmod +x setup-latest.sh
RUN /setup-latest.sh

RUN adduser --disabled-password --gecos "Link Parser User" link-parser

USER link-parser
WORKDIR /home/link-parser
CMD bash

# echo "this is a test" | link-parser


