#
# Dockerfile for developing link-grammar.
#
# Usage:
#    1. From the directory containing this file run
#           docker build --tag="linkgrammar/lgdev" .
#    2. From the root directory of the link-grammar git clone run
#           docker run --rm  -v $PWD:/home/lgd/host -it linkgrammar/lgdev

FROM ubuntu:14.04

RUN apt-get update

# Developer Tools
RUN apt-get -y install git vim rlwrap

# Build tools
RUN apt-get -y install autoconf-archive gcc g++ make libtool libgc-dev \
                       pkg-config

# Create and switch user. The user is privileged with no password required
# Becareful with this one on sharing volume. Only mount user-space directories.
RUN adduser --disabled-password --gecos 'Link-Grammar Developer' lgd && \
    adduser lgd sudo && echo 'lgd  ALL=(ALL) NOPASSWD:ALL ' >> /etc/sudoers

USER lgd
WORKDIR /home/lgd
CMD bash
