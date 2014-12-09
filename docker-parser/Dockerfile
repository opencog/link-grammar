#
# Dockerfile for the link-grammar parser.
# Allows one to just build and compile the command-line tool,
# and so perform general experimentation.
#
FROM linkgrammar/lgbase:latest
MAINTAINER linasvepstas@gmail.com

# Build the sources.
ADD build-parser.sh /
RUN chmod +x build-parser.sh
RUN /build-parser.sh

RUN adduser --disabled-password --gecos "Link Parser User" link-parser

USER link-parser
WORKDIR /home/link-parser
RUN echo "export LANG=en_US.UTF-8" >> .bash_aliases
CMD bash

RUN export LANG=en_US.UTF-8

RUN echo "this is a test" | link-parser
# 

