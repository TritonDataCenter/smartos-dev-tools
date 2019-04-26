/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2019, Joyent, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <libnvpair.h>
#include <string.h>
#include <stropts.h>
#include <unistd.h>
#include <sys/ddi_ufm.h>
#include <sys/types.h>

#define	UFMDEV	"/dev/ufm"

static const char *pname;
static const char optstr[] = "d:i:";

static void
usage()
{
	(void) fprintf(stderr, "usage: %s -d <devpath> -i <ioctl>\n\n"
	    "where \'ioctl\' can be:\n"
	    "getcaps, reportsz, report\n\n", pname);
}

static int
do_getcaps(int fd, int cmd, char *devpath)
{
	ufm_ioc_getcaps_t ioc = { 0 };

	ioc.ufmg_version = DDI_UFM_CURRENT_VERSION;
	(void) strcpy(ioc.ufmg_devpath, devpath);

	if (ioctl(fd, cmd, &ioc) < 0) {
		(void) fprintf(stderr, "getcaps ioctl failed (%s)\n",
		    strerror(errno));
		return (-1);
	}

	(void) printf("CAPS: %u\n", ioc.ufmg_caps);
	if ((ioc.ufmg_caps & DDI_UFM_CAP_REPORT) == 0) {
		(void) printf("Does not support DDI_UFM_CAP_REPORT\n");
	}

	return (0);
}

static int
do_reportsz(int fd, int cmd, char *devpath)
{
	ufm_ioc_bufsz_t ioc = { 0 };

	ioc.ufbz_version = DDI_UFM_CURRENT_VERSION;
	(void) strcpy(ioc.ufbz_devpath, devpath);

	if (ioctl(fd, cmd, &ioc) < 0) {
		(void) fprintf(stderr, "reportsz ioctl failed (%s)\n",
		    strerror(errno));
		return (-1);
	}

	(void) printf("Report Size: %u bytes\n", ioc.ufbz_size);

	return (ioc.ufbz_size);
}


static int
do_report(int fd, int cmd, char *devpath)
{
	ufm_ioc_report_t ioc = { 0 };
	nvlist_t *nvl = NULL, **images;
	uint_t nimages;

	if ((ioc.ufmr_bufsz = (size_t)do_reportsz(fd, UFM_IOC_REPORTSZ,
	    devpath)) < 0)
		return (-1);

	if ((ioc.ufmr_buf = malloc(ioc.ufmr_bufsz)) == NULL)
		return (-1);

	ioc.ufmr_version = DDI_UFM_CURRENT_VERSION;
	(void) strcpy(ioc.ufmr_devpath, devpath);

	if (ioctl(fd, cmd, &ioc) < 0) {
		(void) fprintf(stderr, "report ioctl failed (%s)\n",
		    strerror(errno));
		return (-1);
	}

	if (nvlist_unpack(ioc.ufmr_buf, ioc.ufmr_bufsz, &nvl,
	    NV_ENCODE_NATIVE) != 0) {
		(void) fprintf(stderr, "failed to unpack nvlist");
		free(ioc.ufmr_buf);
		return (-1);
	}
	free(ioc.ufmr_buf);

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
				return (2);
			}
		}
	}

	if (ioc == NULL || devpath == NULL) {
		(void) fprintf(stderr, "-d/-i options are required\n");
		usage();
		return (2);
	}

	if (strcasecmp(ioc, "getcaps") == 0) {
		cmd = UFM_IOC_GETCAPS;
	} else if (strcmp(ioc, "reportsz") == 0) {
		cmd = UFM_IOC_REPORTSZ;
	} else if (strcmp(ioc, "report") == 0) {
		cmd = UFM_IOC_REPORT;
	} else {
		(void) fprintf(stderr, "invalid ioctl name\n");
		usage();
		return (2);
	}

	if ((fd = open(UFMDEV, O_RDWR)) < 0) {
		(void) fprintf(stderr, "failed to open %s (%s)\n", UFMDEV,
		    strerror(errno));
		return (1);
	}

	switch (cmd) {
	case UFM_IOC_GETCAPS:
		ret = do_getcaps(fd, cmd, devpath);
		break;
	case UFM_IOC_REPORTSZ:
		ret = do_reportsz(fd, cmd, devpath);
		break;
	case UFM_IOC_REPORT:
		ret = do_report(fd, cmd, devpath);
		break;
	default:
		break;
	}

out:
	(void) close(fd);

	return ((ret < 0) ? 0 : 1);
}
