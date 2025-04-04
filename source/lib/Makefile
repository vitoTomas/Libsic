# Compile SimpleContainer source
libsic-compile:
	gcc -c -fPIC simple_container.c -o simple_container.o

# Generate the SO library
libsic-lib: libsic-compile
	gcc -shared -o libsic.so simple_container.o

# Install libsic to proper location
libsic-install: libsic-lib
	cp libsic.so /usr/lib/

clean:
	rm -f simpele_container.o
	rm -f libsic.so
