# Top-level Makefile for pommed & tools

all: pommed gpomme wmpomme

pommed:
	$(MAKE) -C pommed

gpomme:
	$(MAKE) -C gpomme

wmpomme:
	$(MAKE) -C wmpomme

clean:
	$(MAKE) -C pommed clean
	$(MAKE) -C gpomme clean
	$(MAKE) -C wmpomme clean

.PHONY: pommed gpomme wmpomme
