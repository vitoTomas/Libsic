#!/bin/bash

set -ep

VERSION=$(git describe --tags | sed -n 's/^v\([0-9]\.[0-9]\.[0-9]\).*/\1/p')

./terminator
rm -f /usr/lib/libsic.so."$VERSION"
rm -f /usr/lib/libsic.so
rm -rf /tmp/scontainer
make clean
