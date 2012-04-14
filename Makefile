# Top-level Makefile for pommed & tools

OFLIB ?=

all: pommed wmpomme

pommed:
	$(MAKE) -C pommed OFLIB=$(OFLIB)

wmpomme:
	$(MAKE) -C wmpomme

clean:
	$(MAKE) -C pommed clean
	$(MAKE) -C wmpomme clean
	rm -f *~

.PHONY: pommed wmpomme
