# Top-level Makefile for pommed & tools

OFLIB ?=

all: pommed gpomme wmpomme

pommed:
	$(MAKE) -C pommed OFLIB=$(OFLIB)

gpomme:
	$(MAKE) -C gpomme

wmpomme:
	$(MAKE) -C wmpomme

clean:
	$(MAKE) -C pommed clean
	$(MAKE) -C gpomme clean
	$(MAKE) -C wmpomme clean
	rm -f *~

.PHONY: pommed gpomme wmpomme
