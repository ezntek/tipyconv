CC ?= cc
LD ?= ld
INCLUDE = 

SRC = tipyconv.c deserialize.c serialize.c
OBJ = tipyconv.o deserialize.o serialize.o
3RDPARTY_OBJ = 3rdparty/asv/a_string.o 
HEADERS = common.h

RELEASE_CFLAGS = -O2 -Wall -Wextra -pedantic $(INCLUDE) 
DEBUG_CFLAGS = -O0 -g -Wall -Wextra -pedantic -fno-stack-protector -fsanitize=address $(INCLUDE)
TARBALLFILES = Makefile LICENSE.md README.md $(SRC) 

TARGET=debug

ifeq ($(TARGET),debug)
	CFLAGS=$(DEBUG_CFLAGS)
else
	CFLAGS=$(RELEASE_CFLAGS)
endif


tipyconv: setup $(OBJ)
	$(CC) $(LIBS) $(CFLAGS) -o tipyconv $(OBJ) $(3RDPARTY_OBJ)

setup: deps

dep_asv:
	mkdir -p 3rdparty/include
	if [ ! -d 3rdparty/asv ]; then \
		curl -fL -o "3rdparty/asv.zip" "https://github.com/ezntek/asv/archive/refs/heads/main.zip"; \
		unzip 3rdparty/asv.zip -d 3rdparty; \
		mv 3rdparty/asv-main 3rdparty/asv; \
	fi

	test -f 3rdparty/asv/asv.o || $(MAKE) -C 3rdparty/asv
	cp 3rdparty/asv/*.h 3rdparty/include/

deps: dep_asv

cleandeps: 
	rm -rf 3rdparty/*

updatedeps: cleandeps
	$(MAKE) deps

tarball: deps
	mkdir -p tipyconv
	cp -r $(TARBALLFILES) tipyconv/
	tar czf tipyconv.tar.gz tipyconv
	rm -rf tipyconv

distclean: clean cleandeps

clean:
	rm -rf tipyconv tipyconv.tar.gz tipyconv $(OBJ)

.PHONY: clean cleanall
