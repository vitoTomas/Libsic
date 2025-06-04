#!/bin/bash
set -ep

VERSION=$(git describe --tags | sed -n 's/^v\([0-9]\.[0-9]\.[0-9]\).*/\1/p')

echo "Library version v$VERSION"

make default

cp ../source/lib/libsic.so."$VERSION" /usr/lib
ln -s /usr/lib/libsic.so."$VERSION" /usr/lib/libsic.so

rm -rf /tmp/scontainer
./generator
