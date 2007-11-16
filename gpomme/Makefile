CC = gcc

GTK_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-2.0)

GLADE_CFLAGS = $(shell pkg-config --cflags libglade-2.0)
GLADE_LIBS = $(shell pkg-config --libs libglade-2.0)

DBUS_CFLAGS = $(shell pkg-config dbus-1 --cflags) -DDBUS_API_SUBJECT_TO_CHANGE
DBUS_LIBS = $(shell pkg-config dbus-1 --libs)

CONFUSE_CFLAGS = $(shell pkg-config libconfuse --cflags)
CONFUSE_LIBS = $(shell pkg-config libconfuse --libs)

INOTIFY_CFLAGS = $(shell test -e /usr/include/sys/inotify.h || echo -DNO_SYS_INOTIFY_H)

CFLAGS = -g -O2 -Wall $(DBUS_CFLAGS) $(GTK_CFLAGS) $(CONFUSE_CFLAGS) $(GLADE_CFLAGS)
LDFLAGS = -lpthread $(DBUS_LIBS) $(GTK_LIBS) $(CONFUSE_LIBS) $(GLADE_LIBS)

SOURCES = gpomme.c theme.c conffile.c ../dbus-client/dbus-client.c
POFILES = po/fr.po po/de.po po/es.po po/it.po

OBJS = $(SOURCES:%.c=%.o)
MOFILES = $(POFILES:%.po=%.mo)

all: gpomme mo

mo: $(MOFILES)

gpomme: $(OBJS) $(LIBS)

gpomme.o: gpomme.c gpomme.h theme.h ../dbus-client/dbus-client.h

theme.o: theme.c theme.h gpomme.h

conffile.o: conffile.c conffile.h gpomme.h theme.h

../dbus-client/dbus-client.o: ../dbus-client/dbus-client.c ../dbus-client/dbus-client.h

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
	rm -f *~ po/*~
