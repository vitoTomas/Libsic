DIR_LIBSIC = ../source/lib

default: binarylib terminator

binarylib: $(DIR_LIBSIC)/libsic.so
	gcc generator.c -o generator -I../interface -L../source/lib -lsic

terminator: $(DIR_LIBSIC)/libsic.so
	gcc terminator.c -o terminator -I../interface -L../source/lib -lsic

$(DIR_LIBSIC)/simple_container.o:
	make -C $(DIR_LIBSIC) libsic-compile

$(DIR_LIBSIC)/libsic.so:
	make -C $(DIR_LIBSIC) libsic

clean:
	rm -f generator
	rm -f terminator
	make -C $(DIR_LIBSIC) clean
