# Top-level Makefile for pommed

OFLIB ?=

all: pommed-light

pommed-light:
	$(MAKE) -C pommed-light OFLIB=$(OFLIB)

clean:
	$(MAKE) -C pommed-light clean
	rm -f *~

.PHONY: pommed-light
