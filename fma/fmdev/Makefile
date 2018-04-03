#
# Copyright (c) 2018, Joyent, Inc.
#
PROG=		fmdev	
CC=		/opt/local/bin/cc
CTFCONVERT=	/opt/onbld/bin/i386/ctfconvert
CTFMERGE=	/opt/onbld/bin/i386/ctfmerge
#
# This can optionally be overridden to headers and/or libraries from a built
# illumos proto area.
#
PROTO=		/

LDFLAGS=	-L$(PROTO)/lib -lnvpair
CFLAGS=		-I$(PROTO)/include -g -std=gnu99

SRCS=	fmdev.c
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

