#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright (c) 2018, Joyent, Inc.
#
PROG=		32/dump-sp-info
PROG64=		64/dump-sp-info
CC=		/opt/local/bin/cc
#
# This can optionally be overridden to the proto area of an illumos repo
#
PROTO=		/

LDFLAGS=	-L$(PROTO)/usr/lib -lipmi -lnvpair -lsocket -lnsl
CFLAGS=		-g -std=gnu99 -I $(PROTO)/usr/include

$(PROG): dump-sp-info.c
	mkdir -p 32
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(PROG64): dump-sp-info.c
	mkdir -p 64
	$(CC) -m64 $(CFLAGS) $(LDFLAGS) -o $@ $<

all: $(PROG) $(PROG64)

clean clobber:
	$(RM) $(PROG) $(PROG64)
