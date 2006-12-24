CC = gcc

DBUS_CFLAGS = $(shell pkg-config dbus-1 --cflags)
DBUS_LIBS = $(shell pkg-config dbus-1 --libs)

CFLAGS = -g -O2 -Wall $(DBUS_CFLAGS)
LDFLAGS = -lXpm -lXext -lX11 $(DBUS_LIBS)

SOURCES = wmgeneral.c wmpomme.c ../dbus-client/dbus-client.c

OBJS = $(SOURCES:%.c=%.o)

wmpomme: $(OBJS)

wmpomme.o: wmpomme.c wmgeneral.h wmpomme-master.xpm ../dbus-client/dbus-client.h

wmgeneral.o: wmgeneral.c wmgeneral.h

../dbus-client/dbus-client.o: ../dbus-client/dbus-client.c ../dbus-client/dbus-client.h

clean:
	rm -f wmpomme $(OBJS)
	rm -f *~
