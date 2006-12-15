CC = gcc
CFLAGS = -g -O2 -Wall
LDFLAGS = -lz -lsmbios -lconfuse -lasound

LIBS = /usr/lib/libpci.a

SOURCES = mbpeventd.c kbd_backlight.c ambient.c cd_eject.c evdev.c \
		x1600_backlight.c gma950_backlight.c conffile.c audio.c

OBJS = $(SOURCES:%.c=%.o)

mbpeventd: $(OBJS) $(LIBS)

mbpeventd.o: mbpeventd.c mbpeventd.h kbd_backlight.h lcd_backlight.h cd_eject.h evdev.h conffile.h audio.h

kbd_backlight.o: kbd_backlight.c kbd_backlight.h mbpeventd.h ambient.h conffile.h

ambient.o: ambient.c ambient.h mbpeventd.h

cd_eject.o: cd_eject.c cd_eject.h mbpeventd.h conffile.h

evdev.o: evdev.c evdev.h mbpeventd.h kbd_backlight.h lcd_backlight.h cd_eject.h conffile.h audio.h

x1600_backlight.o: x1600_backlight.c mbpeventd.h lcd_backlight.h conffile.h

gma950_backlight.o: gma950_backlight.c mbpeventd.h lcd_backlight.h conffile.h

conffile.o: conffile.c conffile.h mbpeventd.h lcd_backlight.h kbd_backlight.h cd_eject.h

audio.o: audio.c audio.h mbpeventd.h

clean:
	rm -f mbpeventd $(OBJS)
	rm -f *~
