CC = gcc

GTK_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-2.0)

DBUS_CFLAGS = $(shell pkg-config dbus-1 --cflags) -DDBUS_API_SUBJECT_TO_CHANGE
DBUS_LIBS = $(shell pkg-config dbus-1 --libs)

DBUSGLIB_CFLAGS = $(shell pkg-config dbus-glib-1 --cflags)
DBUSGLIB_LIBS = $(shell pkg-config dbus-glib-1 --libs)

CONFUSE_CFLAGS = $(shell pkg-config libconfuse --cflags)
CONFUSE_LIBS = $(shell pkg-config libconfuse --libs)

INOTIFY_CFLAGS = $(shell test -e /usr/include/sys/inotify.h || echo -DNO_SYS_INOTIFY_H)

CFLAGS = -g -O2 -Wall $(DBUS_CFLAGS) $(DBUSGLIB_CFLAGS) $(GTK_CFLAGS) $(CONFUSE_CFLAGS) $(INOTIFY_CFLAGS)
LDLIBS = -lpthread -lX11 $(DBUS_LIBS) $(DBUSGLIB_LIBS) $(GTK_LIBS) $(CONFUSE_LIBS)

SOURCES = gpomme.c theme.c conffile.c \
		../client-common/dbus-client.c \
		../client-common/video-client.c
POFILES = po/fr.po po/de.po po/es.po po/it.po po/ja.po

OBJS = $(SOURCES:%.c=%.o)
MOFILES = $(POFILES:%.po=%.mo)

all: gpomme mo

mo: $(MOFILES)

gpomme: $(OBJS)

gpomme.o: gpomme.c gpomme.h theme.h ../client-common/dbus-client.h ../client-common/video-client.h

theme.o: theme.c theme.h gpomme.h

conffile.o: conffile.c conffile.h gpomme.h theme.h

../client-common/dbus-client.o: ../client-common/dbus-client.c ../client-common/dbus-client.h

../client-common/video-client.o: ../client-common/video-client.c ../client-common/video-client.h

%.mo: %.po
	@echo Building $@...
	@msgfmt --statistics -o $@ $<

updatepo:
	xgettext -L C -k_ -o po/gpomme.pot $(SOURCES)
	for po in $(POFILES); do \
		msgmerge -U $$po po/gpomme.pot; \
	done

clean:
	rm -f gpomme $(OBJS) $(MOFILES)
	rm -f *~ po/*~ ../client-common/*~
