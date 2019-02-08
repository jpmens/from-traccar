# include config.mk

# StatsD host is not optional
STATSDHOST="127.0.0.1"

CFLAGS= -Wall -Werror
LDFLAGS	= $(MORELIBS) -lm -L. -lmosquitto -lmicrohttpd

ifneq ($(origin STATSDHOST), undefined)
	CFLAGS += -DSTATSDHOST=\"$(STATSDHOST)\"
endif


TARGETS=
FT_OBJS = json.o
FT_EXTRA_OBJS = statsd/statsd-client.o

ifneq ($(FREEBSD),yes)
else
endif

TARGETS += ft

GIT_VERSION := $(shell git describe --long --abbrev=10 --dirty --tags 2>/dev/null || echo "tarball")
CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"

all: $(TARGETS)

ft: ft.o $(FT_OBJS) $(FT_EXTRA_OBJS)
	$(CC) $(CFLAGS) -o ft ft.o $(FT_OBJS) $(FT_EXTRA_OBJS) $(LDFLAGS)
	if test -r codesign.sh; then /bin/sh codesign.sh; fi

$(FT_OBJS): Makefile json.h utstring.h

ft.o: ft.c Makefile 

clean:
	rm -f *.o

clobber: clean
	rm -f ft
