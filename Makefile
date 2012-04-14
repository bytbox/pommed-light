# Top-level Makefile for pommed

OFLIB ?=

all: pommed

pommed:
	$(MAKE) -C pommed OFLIB=$(OFLIB)

clean:
	$(MAKE) -C pommed clean
	rm -f *~

.PHONY: pommed
