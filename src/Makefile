CC = gcc
CFLAGS = -g -O2 -Wall

SOURCES = mbpeventd.c kbd_backlight.c ambient.c
OBJS = $(SOURCES:%.c=%.o)

mbpeventd: $(OBJS)

mbpeventd.o: mbpeventd.c mbpeventd.h kbd_backlight.h ambient.h

kbd_backlight.o: kbd_backlight.c mbpeventd.h kbd_backlight.h ambient.h

ambient.o: ambient.c mbpeventd.h ambient.h

clean:
	rm -f mbpeventd $(OBJS)
	rm -f *~
