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
#include <libipmi.h>
#include <libnvpair.h>
#include <string.h>
#include <sys/types.h>

static const char *pname;
static const char optstr[] = "e:h:i:n:p:u:t:";

static void
usage()
{
	(void) fprintf(stderr, "usage: %s [-t <bmc|lan>] [-h host] [-u user] "
	    "[-p passwd] -n entity_name -e entity_id, -i entity_inst\n\n",
	    pname);
}

int
main(int argc, char **argv)
{
	ipmi_handle_t *ihp;
	char *errmsg, *e_name = NULL;
	uint_t xport_type = IPMI_TRANSPORT_BMC;
	char c, *host = NULL, *user = NULL, *passwd = NULL;
	int err, status = 1;
	nvlist_t *params = NULL;
	ipmi_sdr_full_sensor_t *fsensor;
	ipmi_sdr_compact_sensor_t *csensor;
	ipmi_sdr_t *sdr = NULL;
	ipmi_sensor_reading_t *reading;
	double conv_reading;
	uint8_t sensor_num, state, e_id = -1, e_inst = -1;
	boolean_t is_threshold = B_FALSE;

	pname = argv[0];
	while (optind < argc) {
		while ((c = getopt(argc, argv, optstr)) != -1) {
			switch (c) {
			case 'e':
				e_id = strtol(optarg, NULL, 0);
				break;
			case 'h':
				host = optarg;
				break;
			case 'i':
				e_inst = strtol(optarg, NULL, 0);
				break;
			case 'n':
				e_name = optarg;
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
	if (e_name == NULL || e_id == -1 || e_inst == -1) {
		(void) fprintf(stderr, "-e/-i/-n are required args\n");
		usage();
		return (2);
	}
	if ((ihp = ipmi_open(&err, &errmsg, xport_type, params)) == NULL) {
		(void) fprintf(stderr, "failed to open libipmi: %s\n",
		    errmsg);
		return (1);
	}

	if ((sdr = ipmi_sdr_lookup_precise(ihp, e_name, e_id, e_inst)) ==
	    NULL) {
		(void) fprintf(stderr, "Failed to lookup SDR for %s (%s)\n",
		    e_name, ipmi_errmsg(ihp));
		goto out;
	}
	switch (sdr->is_type) {
		case IPMI_SDR_TYPE_FULL_SENSOR:
			fsensor = (ipmi_sdr_full_sensor_t *)sdr->is_record;
			sensor_num = fsensor->is_fs_number;
			if (fsensor->is_fs_reading_type == IPMI_RT_THRESHOLD) {
				is_threshold = B_TRUE;
			}
			break;
		case IPMI_SDR_TYPE_COMPACT_SENSOR:
			csensor = (ipmi_sdr_compact_sensor_t *)sdr->is_record;
			sensor_num = csensor->is_cs_number;
			if (csensor->is_cs_reading_type == IPMI_RT_THRESHOLD) {
				is_threshold = B_TRUE;
			}
			break;
		default:
			(void) fprintf(stderr, "%s does not refer to a full "
			    "or compact SDR\n", e_name);
			return (-1);
	}
	if ((reading = ipmi_get_sensor_reading(ihp, sensor_num)) == NULL) {
		(void) fprintf(stderr, "Failed to get sensor reading for "
		    "sensor %s, sensor_num=%d (%s)\n", e_name, sensor_num,
		    ipmi_errmsg(ihp));
		goto out;
	}
	if (is_threshold == B_TRUE &&
	    sdr->is_type == IPMI_SDR_TYPE_FULL_SENSOR) {
		if (ipmi_sdr_conv_reading(fsensor, reading->isr_reading,
		    &conv_reading) != 0) {
			(void) fprintf(stderr, "Failed to convert sensor "
			    "reading for sensor %s (%s)\n", e_name,
			    ipmi_errmsg(ihp));
			goto out;
		} else
			(void) printf("reading: %lf\n", conv_reading);
	}
	(void) printf("state: 0x%04x\n", reading->isr_state);
	status = 0;
out:
	ipmi_close(ihp);

	return (status);
}
