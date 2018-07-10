#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright (c) 2018, Joyent, Inc.
#
PROG=		32/read-sensor
PROG64=		64/read-sensor
CC=		/opt/local/bin/cc
PROTO=		/
CFLAGS=		-g -std=gnu99 -I$(PROTO)/usr/include
LDFLAGS=	-L$(PROTO)/usr/lib -lipmi -lnvpair
LDFLAGS64=	-L$(PROTO)/usr/lib/64 -lipmi -lnvpair

$(PROG): read-sensor.c
	mkdir -p 32
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $<

$(PROG64): read-sensor.c
	mkdir -p 64
	$(CC) -m64 $(CFLAGS) $(LDFLAGS64) -o $@ $<

all: $(PROG) $(PROG64)

clean clobber:
	$(RM) $(PROG) $(PROG64)
