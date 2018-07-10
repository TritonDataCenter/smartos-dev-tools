#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright (c) 2018, Joyent, Inc.
#
PROG=		gen-diagcode
CC=		/opt/local/bin/cc
CTFCONVERT=	/opt/onbld/bin/i386/ctfconvert
CTFMERGE=	/opt/onbld/bin/i386/ctfmerge
#
# This can optionally be overridden to reference headers and/or libraries from
# a built illumos proto area.
#
PROTO=		/

LDFLAGS=	-ldiagcode -L$(PROTO)/usr/lib/fm -R/usr/lib/fm
CFLAGS=		-g -std=gnu99 -I$(PROTO)/usr/include

SRCS=	gen_diagcode.c
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

