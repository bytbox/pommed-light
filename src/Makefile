CC = gcc
CFLAGS = -g -O2 -Wall
LDFLAGS = -lz -lsmbios

LIBS = /usr/lib/libpci.a

SOURCES = mbpeventd.c kbd_backlight.c ambient.c cd_eject.c x1600_backlight.c \
		evdev.c

OBJS = $(SOURCES:%.c=%.o)

mbpeventd: $(OBJS) $(LIBS)

mbpeventd.o: mbpeventd.c mbpeventd.h

kbd_backlight.o: kbd_backlight.c mbpeventd.h kbd_backlight.h ambient.h

ambient.o: ambient.c mbpeventd.h ambient.h

cd_eject.o: cd_eject.c mbpeventd.h cd_eject.h

x1600_backlight.o: x1600_backlight.c mbpeventd.h lcd_backlight.h

evdev.o: evdev.c mbpeventd.h kbd_backlight.h ambient.h cd_eject.h evdev.h

clean:
	rm -f mbpeventd $(OBJS)
	rm -f *~
