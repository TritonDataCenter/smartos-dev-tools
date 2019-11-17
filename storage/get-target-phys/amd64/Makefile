#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2019 Joyent, Inc.
#
include ../Makefile.com

%.o: ../%.c
	$(CC) $(CFLAGS) -m64 -c $< -o $@
	$(CTFCONVERT) -l 0 $@

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -m64 -o $@ $(OBJS)
	$(CTFMERGE) -l 0 -o $@ $(OBJS)
