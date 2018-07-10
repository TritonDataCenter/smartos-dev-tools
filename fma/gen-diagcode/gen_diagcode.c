/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018, Joyent, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stropts.h>
#include <unistd.h>
#include <fm/diagcode.h>
#include <sys/types.h>

#define	DICTDIR	"usr/lib/fm/dict"

static const char *pname;
static const char optstr[] = "d:f:R:";

static void
usage()
{
	(void) fprintf(stderr, "usage: %s [-R rootdir] -d <dict> "
	    "-f <FM class>\n\n", pname);
}

int
main(int argc, char **argv)
{
	char c, *dict = NULL, *fm_class = NULL, *root = "/", code[64];
	char dictpath[MAXPATHLEN + 1];
	int status = 1;
	fm_dc_handle_t *dhp = NULL;
	const char *key[2];

	pname = argv[0];
	while (optind < argc) {
		while ((c = getopt(argc, argv, optstr)) != -1) {
			switch (c) {
			case 'd':
				dict = optarg;
				break;
			case 'f':
				fm_class = optarg;
				break;
			case 'R':
				root = optarg;
				break;
			default:
				usage();
				return (2);
			}
		}
	}

	if (dict == NULL || fm_class == NULL) {
		(void) fprintf(stderr, "-d/-f options are required\n");
		usage();
		return (2);
	}
	(void) snprintf(dictpath, MAXPATHLEN, "%s/%s", root, DICTDIR);
	if ((dhp = fm_dc_opendict(FM_DC_VERSION, dictpath, dict)) == NULL) {
		(void) fprintf(stderr, "fm_dc_opendict failed for %s\n",
		    dictpath);
		goto out;
	}

	key[0] = fm_class;
	key[1] = NULL;
	if (fm_dc_key2code(dhp, key, code, sizeof (code)) < 0) {
		(void) fprintf(stderr, "fm_dc_key2code failed for %s\n", key[0]);
		goto out;
	}
	(void) printf("%s\n", code);
	status = 0;
out:
	if (dhp != NULL)
		fm_dc_closedict(dhp);
	return (status);
}
