#!/bin/bash
set -ex

pushd "$(dirname "$0")"

pushd link-parser/dist
npm publish
popd

popd
