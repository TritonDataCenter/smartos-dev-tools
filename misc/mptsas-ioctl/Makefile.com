#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 2019 Joyent, Inc.
#
PROG=		mptsas-ioctl
CC=		$(shell which gcc)
ARCH=		$(shell uname -p)
CTFTOOLS=	/opt/onbld/bin/$(ARCH)
CTFCONVERT=	$(CTFTOOLS)/ctfconvert
CTFMERGE=	$(CTFTOOLS)/ctfmerge

PROTO=		/
CFLAGS=		-g -std=gnu99 -I$(PROTO)/usr/include

SRCS= mptsas-ioctl.c

OBJS = $(SRCS:%.c=%.o)

all: $(PROG)

clean clobber:
	$(RM) $(PROG) $(OBJS)
