CC = gcc

DBUS_CFLAGS = $(shell pkg-config dbus-1 --cflags) -DDBUS_API_SUBJECT_TO_CHANGE
DBUS_LIBS = $(shell pkg-config dbus-1 --libs)

TIMERFD_CFLAGS = $(shell test -e /usr/include/sys/timerfd.h || echo -DNO_SYS_TIMERFD_H)

CFLAGS = -g -O2 -Wall $(DBUS_CFLAGS) $(TIMERFD_CFLAGS)
LDFLAGS = -lrt -lXpm -lXext -lX11 $(DBUS_LIBS)

SOURCES = wmgeneral.c wmpomme.c \
	../client-common/dbus-client.c \
	../client-common/video-client.c

OBJS = $(SOURCES:%.c=%.o)

wmpomme: $(OBJS)

wmpomme.o: wmpomme.c wmgeneral.h wmpomme-master.xpm ../client-common/dbus-client.h

wmgeneral.o: wmgeneral.c wmgeneral.h

../client-common/dbus-client.o: ../client-common/dbus-client.c ../client-common/dbus-client.h

../client-common/dbus-video.o: ../client-common/video-client.c ../client-common/video-client.h

clean:
	rm -f wmpomme $(OBJS)
	rm -f *~ ../client-common/*~
