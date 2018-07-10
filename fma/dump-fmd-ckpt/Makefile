#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright (c) 2018, Joyent, Inc.
#  
PROG=		dump-fmd-ckpt
CC=		/opt/local/bin/cc
CTFCONVERT=	/opt/onbld/bin/i386/ctfconvert
CTFMERGE=	/opt/onbld/bin/i386/ctfmerge

#
# You MUST set this on the commandline to point to the root of an illumos-gate
# or illumos-joyent repo.
#
ON_WS=

FMD_SRC=	usr/src/cmd/fm/fmd/common
CFLAGS=		-g -I $(ON_WS)/$(FMD_SRC)
SRCS=		dump-fmd-ckpt.c
OBJS=		$(SRCS:%.c=%.o)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
	$(CTFCONVERT) -l 0 $@

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS)	
	$(CTFMERGE) -l 0 -o $@ $(OBJS)

all: $(PROG)

clean clobber:
	$(RM) $(PROG) $(OBJS)

