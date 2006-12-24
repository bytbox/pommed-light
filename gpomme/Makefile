CC = gcc

GTK_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-2.0)

CFLAGS = -g -O2 -Wall -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include $(GTK_CFLAGS)
LDFLAGS = -lasound -ldbus-1 -laudiofile -lpthread $(GTK_LIBS)

SOURCES = mbpgtk.c theme.c audio.c ../dbus-client/dbus-client.c

OBJS = $(SOURCES:%.c=%.o)

mbpgtk: $(OBJS) $(LIBS)

mbpgtk.o: mbpgtk.c mbpgtk.h theme.h audio.h ../dbus-client/dbus-client.h

theme.o: theme.c theme.h mbpgtk.h

audio.o: audio.c audio.h mbpgtk.h

../dbus-client/dbus-client.o: ../dbus-client/dbus-client.c ../dbus-client/dbus-client.h

clean:
	rm -f mbpgtk $(OBJS)
	rm -f *~
