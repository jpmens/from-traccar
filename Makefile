# include config.mk

CFLAGS	+=-Wall -Werror
LIBS	= $(MORELIBS) -lm -L. -lmosquitto

TARGETS=
FT_OBJS = json.o mongoose.o 
FT_EXTRA_OBJS = 

ifneq ($(FREEBSD),yes)
else
endif

TARGETS += ft

GIT_VERSION := $(shell git describe --long --abbrev=10 --dirty --tags 2>/dev/null || echo "tarball")
CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"

all: $(TARGETS)

ft: ft.o $(FT_OBJS) $(FT_EXTRA_OBJS)
	$(CC) $(CFLAGS) -o ft ft.o $(FT_OBJS) $(FT_EXTRA_OBJS) $(LIBS)
	if test -r codesign.sh; then /bin/sh codesign.sh; fi

$(FT_OBJS): config.mk Makefile json.h

ft.o: ft.c Makefile 

clean:
	rm -f *.o

clobber: clean
	rm -f ft
