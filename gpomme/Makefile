CC = gcc

GTK_CFLAGS = $(shell pkg-config --cflags gtk+-2.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-2.0)

DBUS_CFLAGS = $(shell pkg-config dbus-1 --cflags) -DDBUS_API_SUBJECT_TO_CHANGE
DBUS_LIBS = $(shell pkg-config dbus-1 --libs)

ALSA_CFLAGS = $(shell pkg-config alsa --cflags)
ALSA_LIBS = $(shell pkg-config alsa --libs)

AUDIOFILE_CFLAGS = $(shell pkg-config audiofile --cflags)
AUDIOFILE_LIBS = $(shell pkg-config audiofile --libs)

CFLAGS = -g -O2 -Wall $(ALSA_CFLAGS) $(AUDIOFILE_CFLAGS) $(DBUS_CFLAGS) $(GTK_CFLAGS)
LDFLAGS = -lpthread $(ALSA_LIBS) $(AUDIOFILE_LIBS) $(DBUS_LIBS) $(GTK_LIBS)

SOURCES = gpomme.c theme.c audio.c ../dbus-client/dbus-client.c
POFILES = po/fr.po po/de.po po/es.po

OBJS = $(SOURCES:%.c=%.o)
MOFILES = $(POFILES:%.po=%.mo)

all: gpomme mo

mo: $(MOFILES)

gpomme: $(OBJS) $(LIBS)

gpomme.o: gpomme.c gpomme.h theme.h audio.h ../dbus-client/dbus-client.h

theme.o: theme.c theme.h gpomme.h

audio.o: audio.c audio.h gpomme.h

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
