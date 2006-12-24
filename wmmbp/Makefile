CC = gcc
CFLAGS = -g -O2 -Wall -I../dbus-client -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include
LDFLAGS = -lXpm -lXext -lX11 -ldbus-1

SOURCES = wmgeneral.c wmmbp.c ../dbus-client/dbus-client.c

OBJS = $(SOURCES:%.c=%.o)

wmmbp: $(OBJS)

wmmbp.o: wmmbp.c wmgeneral.h ../dbus-client/dbus-client.h

wmgeneral.o: wmgeneral.c wmgeneral.h

../dbus-client/dbus-client.o: ../dbus-client/dbus-client.c ../dbus-client/dbus-client.h

clean:
	rm -f wmmbp $(OBJS)
	rm -f *~
