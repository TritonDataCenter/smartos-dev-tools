/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2019 Joyent, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <libnvpair.h>
#include <string.h>
#include <stropts.h>
#include <unistd.h>
#include <sys/types.h>

#define	EXIT_USAGE	2

#define	MPTIOCTL			('I' << 8)
#define	MPTIOCTL_GET_ADAPTER_DATA	(MPTIOCTL | 1)
#define	MPTIOCTL_UPDATE_FLASH		(MPTIOCTL | 2)
#define	MPTIOCTL_RESET_ADAPTER		(MPTIOCTL | 3)
#define	MPTIOCTL_PASS_THRU		(MPTIOCTL | 4)
#define	MPTIOCTL_EVENT_QUERY		(MPTIOCTL | 5)
#define	MPTIOCTL_EVENT_ENABLE		(MPTIOCTL | 6)
#define	MPTIOCTL_EVENT_REPORT		(MPTIOCTL | 7)
#define	MPTIOCTL_GET_PCI_INFO		(MPTIOCTL | 8)
#define	MPTIOCTL_DIAG_ACTION		(MPTIOCTL | 9)
#define	MPTIOCTL_REG_ACCESS		(MPTIOCTL | 10)
#define	MPTIOCTL_GET_DISK_INFO		(MPTIOCTL | 11)
#define	MPTIOCTL_LED_CONTROL		(MPTIOCTL | 12)
#define	MPTIOCTL_GET_CONNECTOR_INFO	(MPTIOCTL | 13)

typedef struct mptsas_get_connector_info
{
	size_t			mci_bufsz;
	caddr_t			mci_buf;
} mptsas_get_connector_info_t;

static const char *pname;
static const char optstr[] = "d:i:";

static void
usage()
{
	(void) fprintf(stderr, "usage: %s -d <devpath> -i <ioctl>\n\n"
	    "where \'ioctl\' can be:\n"
	    "getconninfo\n\n", pname);
}

static int
do_getconninfo(int fd, char *devpath)
{
	mptsas_get_connector_info_t ioc = { 0 };
	nvlist_t *nvl = NULL;
	uint_t nphys;

	if (ioctl(fd, MPTIOCTL_GET_CONNECTOR_INFO, &ioc) < 0) {
		(void) fprintf(stderr, "MPTIOCTL_GET_CONNECTOR_INFO ioctl "
		    "failed (%s)\n", strerror(errno));
		return (-1);
	}
	(void) printf("Required buffer size = %u\n", ioc.mci_bufsz);

	if ((ioc.mci_buf = malloc(ioc.mci_bufsz)) == NULL)
		return (-1);

	if (ioctl(fd, MPTIOCTL_GET_CONNECTOR_INFO, &ioc) < 0) {
		(void) fprintf(stderr, "MPTIOCTL_GET_CONNECTOR_INFO ioctl "
		    "failed (%s)\n", strerror(errno));
		return (-1);
	}

	if (nvlist_unpack(ioc.mci_buf, ioc.mci_bufsz, &nvl,
	    NV_ENCODE_NATIVE) != 0) {
		(void) fprintf(stderr, "failed to unpack nvlist");
		free(ioc.mci_buf);
		return (-1);
	}
	free(ioc.mci_buf);

	nvlist_print(stdout, nvl);

	return (0);
}

int
main(int argc, char **argv)
{
	char c, *ioc = NULL, *devpath = NULL;
	int cmd, fd, ret;

	pname = argv[0];
	while (optind < argc) {
		while ((c = getopt(argc, argv, optstr)) != -1) {
			switch (c) {
			case 'd':
				devpath = optarg;
				break;
			case 'i':
				ioc = optarg;
				break;
			default:
				usage();
				return (EXIT_USAGE);
			}
		}
	}

	if (ioc == NULL || devpath == NULL) {
		(void) fprintf(stderr, "-d/-i options are required\n");
		usage();
		return (EXIT_USAGE);
	}

	if (strcasecmp(ioc, "getconninfo") == 0) {
		cmd = MPTIOCTL_GET_CONNECTOR_INFO;
	} else {
		(void) fprintf(stderr, "invalid ioctl name\n");
		usage();
		return (EXIT_USAGE);
	}

	if ((fd = open(devpath, O_RDONLY)) < 0) {
		(void) fprintf(stderr, "failed to open %s (%s)\n", devpath,
		    strerror(errno));
		return (EXIT_FAILURE);
	}

	switch (cmd) {
	case MPTIOCTL_GET_CONNECTOR_INFO:
		ret = do_getconninfo(fd, devpath);
		break;
	default:
		break;
	}

out:
	(void) close(fd);

	return ((ret < 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
