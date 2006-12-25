CC = gcc

DBUS_CFLAGS = $(shell pkg-config dbus-1 --cflags)
DBUS_LIBS = $(shell pkg-config dbus-1 --libs)

ALSA_CFLAGS = $(shell pkg-config alsa --cflags)
ALSA_LIBS = $(shell pkg-config alsa --libs)

CONFUSE_CFLAGS = $(shell pkg-config libconfuse --cflags)
CONFUSE_LIBS = $(shell pkg-config libconfuse --libs)

CFLAGS = -g -O2 -Wall $(CONFUSE_CFLAGS) $(DBUS_CFLAGS) $(ALSA_CFLAGS)
LDFLAGS = -lz -lsmbios $(CONFUSE_LIBS) $(DBUS_LIBS) $(ALSA_LIBS)

LIBS = /usr/lib/libpci.a

SOURCES = pommed.c cd_eject.c evdev.c conffile.c audio.c dbus.c \
		mactel/x1600_backlight.c mactel/gma950_backlight.c \
		mactel/kbd_backlight.c mactel/ambient.c

OBJS = $(SOURCES:%.c=%.o)

pommed: $(OBJS) $(LIBS)

pommed.o: pommed.c pommed.h kbd_backlight.h lcd_backlight.h cd_eject.h evdev.h conffile.h audio.h dbus.h

cd_eject.o: cd_eject.c cd_eject.h pommed.h conffile.h dbus.h

evdev.o: evdev.c evdev.h pommed.h kbd_backlight.h lcd_backlight.h cd_eject.h conffile.h audio.h

conffile.o: conffile.c conffile.h pommed.h lcd_backlight.h kbd_backlight.h cd_eject.h audio.h

audio.o: audio.c audio.h pommed.h conffile.h dbus.h

dbus.o: dbus.c dbus.h pommed.h lcd_backlight.h kbd_backlight.h ambient.h audio.h

mactel/x1600_backlight.o: mactel/x1600_backlight.c pommed.h lcd_backlight.h conffile.h dbus.h

mactel/gma950_backlight.o: mactel/gma950_backlight.c pommed.h lcd_backlight.h conffile.h dbus.h

mactel/kbd_backlight.o: mactel/kbd_backlight.c kbd_backlight.h pommed.h ambient.h conffile.h dbus.h

mactel/ambient.o: mactel/ambient.c ambient.h pommed.h dbus.h

clean:
	rm -f pommed $(OBJS)
	rm -f *~ mactel/*~
