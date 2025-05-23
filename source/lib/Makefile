# Compile SimpleContainer source

VERSION := $(shell git describe --tags | sed 's/^v\(.*\)/\1/')

libsic: libsic-lib

libsic-compile:
	gcc -c -Wall -Werror -fPIC -I../../interface simple_container.c -o simple_container.o

# Generate the SO library
libsic-lib: libsic-compile
	gcc -shared -o libsic.so.$(VERSION) simple_container.o

# Install libsic to proper location
libsic-install: libsic-lib
	cp libsic.so /usr/lib/

clean:
	rm -f simple_container.o
	rm -f libsic.so.$(VERSION)
