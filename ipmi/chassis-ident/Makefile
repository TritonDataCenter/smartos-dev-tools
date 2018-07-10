#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright (c) 2018, Joyent, Inc.
#
PROG=		chassis-ident
CC=		/opt/local/bin/cc
CTFCONVERT=	/opt/onbld/bin/i386/ctfconvert
CTFMERGE=	/opt/onbld/bin/i386/ctfmerge
#
# This can optionally be overridden to reference libipmi header and
# .so from an illumos proto area
#
PROTO=		/

LDFLAGS=	-L$(PROTO)/usr/lib -lipmi -lnvpair
CFLAGS=		-g -std=gnu99 -I$(PROTO)/usr/include

SRCS=	chassis-ident.c
OBJS=	$(SRCS:%.c=%.o)	

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
	$(CTFCONVERT) -l 0 $@

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	$(CTFMERGE) -l 0 -o $@ $(OBJS)

all: $(PROG)

clean clobber:
	$(RM) $(PROG) $(OBJS)

