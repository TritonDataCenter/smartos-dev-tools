/*
 * Copyright (c) 2018, Joyent, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <libipmi.h>
#include <libnvpair.h>
#include <string.h>
#include <sys/types.h>

static const char *pname;
static const char optstr[] = "h:m:p:u:t:";

static void
usage()
{
	(void) fprintf(stderr, "usage: %s -t <bmc|lan> [-h host] [-u user] "
	    "[-p passwd] -m <get|on|off>\n\n", pname);
}

int
main(int argc, char **argv)
{
	ipmi_handle_t *ihp;
	char *errmsg;
	uint_t xport_type = IPMI_TRANSPORT_BMC;
	char c, *host = NULL, *user = NULL, *passwd = NULL, *mode = NULL;
	int err, status = 1;
	nvlist_t *params = NULL;
	boolean_t assert_ident, do_set = B_FALSE;
	ipmi_chassis_status_t *chs;

	pname = argv[0];
	while (optind < argc) {
		while ((c = getopt(argc, argv, optstr)) != -1) {
			switch (c) {
			case 'h':
				host = optarg;
				break;
			case 'm':
				mode = optarg;
				break;
			case 'p':
				passwd = optarg;
				break;
			case 't':
				if (strcmp(optarg, "bmc") == 0)
					xport_type = IPMI_TRANSPORT_BMC;
				else if (strcmp(optarg, "lan") == 0)
					xport_type = IPMI_TRANSPORT_LAN;
				else {
					(void) fprintf(stderr,
					    "ABORT: Invalid transport type\n");
					usage();
					return (2);
				}
				break;
			case 'u':
				user = optarg;
				break;
			default:
				usage();
				return (2);
			}
		}
	}

	if (mode == NULL) {
		(void) fprintf(stderr, "-m option is required\n");
		usage();
		return(2);
	}
	if (strcmp(mode, "on") == 0) {
		assert_ident = B_TRUE;
		do_set = B_TRUE;
	} else if (strcmp(mode, "off") == 0) {
		assert_ident = B_FALSE;
		do_set = B_TRUE;
	} else if (strcmp(mode, "get") != 0) {
		usage();
		return (2);
	}
	if (xport_type == IPMI_TRANSPORT_LAN &&
	    (host == NULL || passwd == NULL || user == NULL)) {
		(void) fprintf(stderr, "-h/-u/-p must all be specified for "
		    "transport type \"lan\"\n");
		usage();
		return (2);
	}
	if (xport_type == IPMI_TRANSPORT_LAN) {
		if (nvlist_alloc(&params, NV_UNIQUE_NAME, 0) ||
		    nvlist_add_string(params, IPMI_LAN_HOST, host) ||
		    nvlist_add_string(params, IPMI_LAN_USER, user) ||
		    nvlist_add_string(params, IPMI_LAN_PASSWD, passwd)) {
			(void) fprintf(stderr,
			    "ABORT: nvlist construction failed\n");
			return (1);
		}
	}
	if ((ihp = ipmi_open(&err, &errmsg, xport_type, params)) == NULL) {
		(void) fprintf(stderr, "failed to open libipmi: %s\n",
		    errmsg);
		return (1);
	}

	if (do_set && ipmi_chassis_identify(ihp, assert_ident) != 0) {
		(void) fprintf(stderr, "chassis identify failed");
	} else {
		if ((chs = ipmi_chassis_status(ihp)) == NULL) {
			(void) fprintf(stderr, "failed to get chassis status\n");
			goto out;
		}
		if (chs->ichs_identify_supported) {
			(void) printf("chassis identify is %s\n",
			    chs->ichs_identify_state ? "on" : "off");
		} else {
			(void) printf("chassis identify status not supported\n");
		}
		free(chs);
	}
	
out:
	ipmi_close(ihp);

	return (status);
}
