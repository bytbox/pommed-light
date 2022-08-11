# Top-level Makefile for pommed

OFLIB ?=
INSTALL = /usr/bin/install
DESTDIR = /usr/local

export INSTALL DESTDIR

all: pommed

pommed:
	$(MAKE) -C pommed OFLIB=$(OFLIB)

clean:
	$(MAKE) -C pommed clean
	rm -f *~

install:
	$(MAKE) -C pommed install

.PHONY: pommed install
