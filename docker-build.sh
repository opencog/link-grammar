#! /bin/bash
#
# Create several diffeerent docker images for the current release of
# link-grammar. This script will download the latest release, and build
# containers for the parser, for the parse-server, and for the python
# bindings.
#
# Must be run as root, or use sudo.
#
cd docker-base
docker build --tag="linkgrammar/lgbase:latest" .
cd ../docker-parser
docker build --tag="linkgrammar/lgparser:latest" .
