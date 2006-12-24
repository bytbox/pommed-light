CC = gcc
CFLAGS = -g -O2 -Wall -I/usr/include/dbus-1.0 -I/usr/lib/dbus-1.0/include
LDFLAGS = -lz -lsmbios -lconfuse -lasound -ldbus-1

LIBS = /usr/lib/libpci.a

SOURCES = mbpeventd.c kbd_backlight.c ambient.c cd_eject.c evdev.c \
		x1600_backlight.c gma950_backlight.c conffile.c audio.c \
		dbus.c

OBJS = $(SOURCES:%.c=%.o)

mbpeventd: $(OBJS) $(LIBS)

mbpeventd.o: mbpeventd.c mbpeventd.h kbd_backlight.h lcd_backlight.h cd_eject.h evdev.h conffile.h audio.h dbus.h

kbd_backlight.o: kbd_backlight.c kbd_backlight.h mbpeventd.h ambient.h conffile.h dbus.h

ambient.o: ambient.c ambient.h mbpeventd.h

cd_eject.o: cd_eject.c cd_eject.h mbpeventd.h conffile.h dbus.h

evdev.o: evdev.c evdev.h mbpeventd.h kbd_backlight.h lcd_backlight.h cd_eject.h conffile.h audio.h

x1600_backlight.o: x1600_backlight.c mbpeventd.h lcd_backlight.h conffile.h dbus.h

gma950_backlight.o: gma950_backlight.c mbpeventd.h lcd_backlight.h conffile.h dbus.h

conffile.o: conffile.c conffile.h mbpeventd.h lcd_backlight.h kbd_backlight.h cd_eject.h audio.h

audio.o: audio.c audio.h mbpeventd.h conffile.h dbus.h

dbus.o: dbus.c dbus.h mbpeventd.h lcd_backlight.h kbd_backlight.h ambient.h audio.h

clean:
	rm -f mbpeventd $(OBJS)
	rm -f *~
