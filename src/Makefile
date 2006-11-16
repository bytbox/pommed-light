CC = gcc
CFLAGS = -g -O2 -Wall

SOURCES = mbpeventd.c kbd_backlight.c ambient.c
OBJS = $(SOURCES:%.c=%.o)

mbpeventd: $(OBJS)

mbpeventd.o: mbpeventd.h kbd_backlight.h ambient.h

kbd_backlight.o: mbpeventd.h kbd_backlight.h ambient.h

ambient.o: mbpeventd.h ambient.h

clean:
	rm -f mbpeventd $(OBJS)
	rm -f *~
