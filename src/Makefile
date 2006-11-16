CC = gcc
CFLAGS = -g -O2 -Wall

SOURCES = mbpeventd.c kbd_backlight.c ambient.c cd_eject.c
OBJS = $(SOURCES:%.c=%.o)

mbpeventd: $(OBJS)

mbpeventd.o: mbpeventd.c mbpeventd.h kbd_backlight.h ambient.h cd_eject.h

kbd_backlight.o: kbd_backlight.c mbpeventd.h kbd_backlight.h ambient.h

ambient.o: ambient.c mbpeventd.h ambient.h

cd_eject.o: cd_eject.c mbpeventd.h cd_eject.h

clean:
	rm -f mbpeventd $(OBJS)
	rm -f *~
