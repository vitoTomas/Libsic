#!/bin/bash
set -p

VERSION=$(git describe --tags | sed 's/^v\(.*\)/\1/')

cp ../source/lib/libsic.so."$VERSION" /usr/lib
ln -s /usr/lib/libsic.so."$VERSION" /usr/lib/libsic.so

make default
rm -rf /tmp/scontainer
./generator
