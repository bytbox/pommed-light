ARCH ?= $(shell uname -m)

CC = gcc

DBUS_CFLAGS = $(shell pkg-config dbus-1 --cflags) -DDBUS_API_SUBJECT_TO_CHANGE
DBUS_LIBS = $(shell pkg-config dbus-1 --libs)

ALSA_CFLAGS = $(shell pkg-config alsa --cflags)
ALSA_LIBS = $(shell pkg-config alsa --libs)

AUDIOFILE_CFLAGS = $(shell pkg-config audiofile --cflags)
AUDIOFILE_LIBS = $(shell pkg-config audiofile --libs)

CONFUSE_CFLAGS = $(shell pkg-config libconfuse --cflags)
CONFUSE_LIBS = $(shell pkg-config libconfuse --libs)

INOTIFY_CFLAGS = $(shell test -e /usr/include/sys/inotify.h || echo -DNO_SYS_INOTIFY_H)

TIMERFD_CFLAGS = $(shell test -e /usr/include/sys/timerfd.h || echo -DNO_SYS_TIMERFD_H)

CFLAGS = -g -O2 -Wall $(DBUS_CFLAGS) $(ALSA_CFLAGS) $(AUDIOFILE_CFLAGS) $(CONFUSE_CFLAGS) $(INOTIFY_CFLAGS) $(TIMERFD_CFLAGS)

LDFLAGS = -pthread -lrt $(DBUS_LIBS) $(ALSA_LIBS) $(AUDIOFILE_LIBS) $(CONFUSE_LIBS)

ifneq (, $(findstring ppc, $(ARCH)))
OFLIB ?=

SOURCES = pommed.c cd_eject.c evdev.c conffile.c audio.c \
		evloop.c dbus.c power.c beep.c video.c \
		sysfs_backlight.c pmac/pmu.c \
		pmac/kbd_backlight.c pmac/ambient.c

OF_SOURCES = pmac/ofapi/of_externals.c pmac/ofapi/of_internals.c \
		pmac/ofapi/of_standard.c

OF_OBJS = $(OF_SOURCES:%.c=%.o)

ifneq ($(OFLIB),)
CFLAGS += -Ipmac
LIBS = pmac/ofapi/oflib.a
else
LDFLAGS += -lofapi
endif

else

LIBPCI_SHARED = $(shell pkg-config libpci && echo true || echo false)

ifeq ($(LIBPCI_SHARED), true)
	LIBPCI_CFLAGS = $(shell pkg-config libpci --cflags)
	LIBPCI_LIBS = $(shell pkg-config libpci --libs)

	CFLAGS += $(LIBPCI_CFLAGS)
	LDFLAGS += $(LIBPCI_LIBS)
else
	LIBS = /usr/lib/libpci.a
	LDFLAGS += -lz	
endif

SOURCES = pommed.c cd_eject.c evdev.c conffile.c audio.c \
		evloop.c dbus.c power.c beep.c video.c \
		sysfs_backlight.c \
		mactel/x1600_backlight.c mactel/gma950_backlight.c \
		mactel/nv8600mgt_backlight.c \
		mactel/kbd_backlight.c mactel/ambient.c mactel/acpi.c
endif

OBJS = $(SOURCES:%.c=%.o)


pommed: $(OBJS) $(LIBS)

pommed.o: pommed.c pommed.h evloop.h kbd_backlight.h lcd_backlight.h cd_eject.h evdev.h conffile.h audio.h dbus.h beep.h

cd_eject.o: cd_eject.c cd_eject.h pommed.h conffile.h dbus.h

evdev.o: evdev.c evdev.h evloop.h pommed.h kbd_backlight.h lcd_backlight.h cd_eject.h conffile.h audio.h video.h beep.h

evloop.o: evloop.c evloop.h pommed.h

conffile.o: conffile.c conffile.h pommed.h lcd_backlight.h kbd_backlight.h cd_eject.h audio.h beep.h

audio.o: audio.c audio.h pommed.h conffile.h dbus.h

dbus.o: dbus.c dbus.h evloop.h pommed.h lcd_backlight.h kbd_backlight.h ambient.h audio.h

power.o: power.c power.h evloop.h pommed.h lcd_backlight.h

beep.o: beep.c beep.h pommed.h evloop.h audio.h

video.o: video.c video.h pommed.h dbus.h

sysfs_backlight.o: sysfs_backlight.c pommed.h lcd_backlight.h conffile.h dbus.h

# PowerMac-specific files
pmac/kbd_backlight.o: pmac/kbd_backlight.c kbd_auto.c kbd_backlight.h evloop.h pommed.h ambient.h conffile.h dbus.h

pmac/ambient.o: pmac/ambient.c ambient.h pommed.h dbus.h

pmac/pmu.o: pmac/pmu.c power.h

# OFlib
pmac/ofapi/oflib.a: $(OF_OBJS)
	ar cru $@ $^

pmac/ofapi/of_externals.o: pmac/ofapi/of_externals.c pmac/ofapi/of_api.h

pmac/ofapi/of_internals.o: pmac/ofapi/of_internals.c pmac/ofapi/of_api.h

pmac/ofapi/of_standard.o: pmac/ofapi/of_standard.c pmac/ofapi/of_api.h


# Mactel-specific files
mactel/x1600_backlight.o: mactel/x1600_backlight.c pommed.h lcd_backlight.h conffile.h dbus.h

mactel/gma950_backlight.o: mactel/gma950_backlight.c pommed.h lcd_backlight.h conffile.h dbus.h

mactel/nv8600mgt_backlight.o: mactel/nv8600mgt_backlight.c pommed.h lcd_backlight.h conffile.h dbus.h

mactel/kbd_backlight.o: mactel/kbd_backlight.c kbd_auto.c kbd_backlight.h evloop.h pommed.h ambient.h conffile.h dbus.h

mactel/ambient.o: mactel/ambient.c ambient.h pommed.h dbus.h

mactel/acpi.o: mactel/acpi.c power.h


clean:
	rm -f pommed $(OBJS) $(OF_OBJS) pmac/ofapi/oflib.a
	rm -f *~ mactel/*~ pmac/*~ pmac/ofapi/*~
