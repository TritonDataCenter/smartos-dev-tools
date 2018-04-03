#
# Copyright (c) 2018, Joyent, Inc.
#
PROG=		topo-indicator
CC=		/opt/local/bin/cc
CTFCONVERT=	/opt/onbld/bin/i386/ctfconvert
CTFMERGE=	/opt/onbld/bin/i386/ctfmerge

CFLAGS=		-g -std=gnu99
LDFLAGS=	-L/usr/lib/fm -ltopo -R/usr/lib/fm -lnvpair

SRCS=	topo-indicator.c
OBJS=	$(SRCS:%.c=%.o)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
	$(CTFCONVERT) -l 0 $@

$(PROG): $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS)
	$(CTFMERGE) -l 0 -o $@ $(OBJS)

all: $(PROG)

clean clobber:
	$(RM) $(PROG) $(OBJS)
