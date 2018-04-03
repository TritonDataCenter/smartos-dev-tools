/*
 * Copyright (c) 2018, Joyent, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <libnvpair.h>
#include <string.h>
#include <stropts.h>
#include <unistd.h>
#include <fm/fmd_agent.h>
#include <sys/devfm.h>
#include <sys/types.h>

#define	FMDEV	"/dev/fm"

static const char *pname;
static const char optstr[] = "i:";

static void
usage()
{
	(void) fprintf(stderr, "usage: %s -i <ioctl>\n\n"
	    "where \'ioctl\' can be:\n"
	    "versions, physcpu_info, gentopo_legacy\n\n", pname);
}

int
main(int argc, char **argv)
{
	char c, *ioc = NULL, *outbuf = NULL;
	fm_ioc_data_t iocdata = { 0 };
	nvlist_t *outnv = NULL;
	int cmd, fd, status = 1;

	pname = argv[0];
	while (optind < argc) {
		while ((c = getopt(argc, argv, optstr)) != -1) {
			switch (c) {
			case 'i':
				ioc = optarg;
				break;
			default:
				usage();
				return (2);
			}
		}
	}

	if (ioc == NULL) {
		(void) fprintf(stderr, "-i option is required\n");
		usage();
		return (2);
	}
	if (strcasecmp(ioc, "versions") == 0) {
		cmd = FM_IOC_VERSIONS;
	} else if (strcmp(ioc, "physcpu_info") == 0) {
		cmd = FM_IOC_PHYSCPU_INFO;
	} else if (strcmp(ioc, "gentopo_legacy") == 0) {
		cmd = FM_IOC_GENTOPO_LEGACY;
	} else {
		(void) fprintf(stderr, "invalid ioctl name\n");
		usage();
		return (2);
	}
	if ((fd = open(FMDEV, O_RDWR)) < 0) {
		(void) fprintf(stderr, "failed to open %s (%s)\n", FMDEV,
		    strerror(errno));
		return (1);
	}
	if ((outbuf = malloc(FM_IOC_OUT_MAXBUFSZ)) == 0) {
		(void) fprintf(stderr, "malloc failed\n");
		goto out;
	}

	iocdata.fid_version = 1;
	iocdata.fid_outbuf = outbuf;
	iocdata.fid_outsz = FM_IOC_OUT_MAXBUFSZ;
	if (ioctl(fd, cmd, &iocdata) < 0) {
		(void) fprintf(stderr, "ioctl failed (%s)\n", strerror(errno));
		goto out;
	}

	if (nvlist_unpack(iocdata.fid_outbuf, iocdata.fid_outsz, &outnv,
	    NV_UNIQUE_NAME) < 0) {
		(void) fprintf(stderr, "failed to unpack nvlist (%s)",
		    strerror(errno));
		goto out;
	}
	nvlist_print(stdout, outnv);
	nvlist_free(outnv);
	status = 0;
out:
	free(outbuf);
	(void) close(fd);

	return (status);
}
