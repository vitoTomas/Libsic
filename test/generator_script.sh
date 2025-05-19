#!/bin/bash
set -p

make default
cp ../source/lib/libsic.so /usr/lib
./generator
