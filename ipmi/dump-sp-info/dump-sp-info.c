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
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

static const char *pname;
static const char optstr[] = "h:p:u:t:";

/*
 * Channel related IPMI commands reserve 4 bits for the channel number.
 */
#define	IPMI_MAX_CHANNEL	0xf

static const char *addr_sources[] = {
	"Unspecified",
	"Static",
	"DHCP",
	"BIOS",
	"Other"
};

static void
usage()
{
	(void) fprintf(stderr, "usage: %s -t <bmc|lan> [-h host] [-u user] "
	    "[-p passwd]\n\n", pname);
}

static int
dump_ipv4_config(ipmi_lan_config_t *lancfg)
{
	char ipv4_addr[INET_ADDRSTRLEN], subnet[INET_ADDRSTRLEN];
	char ipv4_gateway[INET_ADDRSTRLEN];

	if (inet_ntop(AF_INET, &(lancfg->ilc_ipaddr), ipv4_addr,
	    sizeof (ipv4_addr)) == NULL ||
	    inet_ntop(AF_INET, &(lancfg->ilc_subnet), subnet,
	    sizeof (subnet)) == NULL ||
	    inet_ntop(AF_INET, &(lancfg->ilc_gateway_addr), ipv4_gateway,
	    sizeof (ipv4_gateway)) == NULL) {
		(void) fprintf(stderr, "failed to convert IPv4 addresses: %S\n",
		    strerror(errno));
		return (-1);
	}
	(void) printf("\nIPv4 Configuration:\n");
	(void) printf("%-20s%s\n", "Address:", ipv4_addr);
	(void) printf("%-20s%s\n", "Subnet Mask:", subnet);
	(void) printf("%-20s%s\n", "Gateway:", ipv4_gateway);
	(void) printf("%-20s%s\n", "Config Source:",
	    addr_sources[lancfg->ilc_ipaddr_source]);

	return (0);
}

static int
dump_ipv6_config(ipmi_lan_config_t *lancfg)
{
	char ipv6_addr[INET6_ADDRSTRLEN];

	if (inet_ntop(AF_INET6, &(lancfg->ilc_ipv6_addr), ipv6_addr,
	    sizeof (ipv6_addr)) == NULL) {
		(void) fprintf(stderr, "failed to convert IPv6 addresses: %S\n",
		    strerror(errno));
		return (-1);
	}
	(void) printf("\nIPv6 Configuration:\n");
	(void) printf("%-20s%s\n", "Address:", ipv6_addr);
	(void) printf("%-20s%s\n", "Config Source:",
	    addr_sources[lancfg->ilc_ipv6_source]);

	return (0);
}

int
main(int argc, char **argv)
{
	ipmi_handle_t *ihp;
	ipmi_channel_info_t *chinfo;
	ipmi_lan_config_t lancfg = { 0 };
	boolean_t found_lan = B_TRUE;
	char *errmsg;
	const char *sp_ver = NULL;
	uint_t xport_type = IPMI_TRANSPORT_BMC;
	char c, *host = NULL, *user = NULL, *passwd = NULL;
	int err, status = 1, ch;
	nvlist_t *params = NULL;

	pname = argv[0];
	while (optind < argc) {
		while ((c = getopt(argc, argv, optstr)) != -1) {
			switch (c) {
			case 'h':
				host = optarg;
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
	if ((ihp = ipmi_open(&err, &errmsg, xport_type, params)) == NULL) {
		(void) fprintf(stderr, "failed to open libipmi: %s\n",
		    errmsg);
		return (1);
	}

	if ((sp_ver = ipmi_firmware_version(ihp)) == NULL)
		(void) fprintf(stderr, "failed to get firmware version\n");
	else
		(void) printf("%-20s%s\n", "Firmware Version:", sp_ver);

	/*
	 * iterate through the channels to find the LAN channel.
	 */
	for (ch = 0; ch <= IPMI_MAX_CHANNEL; ch++) {
		if ((chinfo = ipmi_get_channel_info(ihp, ch)) != NULL &&
		    chinfo->ici_medium == IPMI_MEDIUM_8023LAN) {
			found_lan = B_TRUE;
			break;
		}
	}
	if (found_lan != B_TRUE ||
	    ipmi_lan_get_config(ihp, ch, &lancfg) != 0) {
		(void) fprintf(stderr, "failed to get LAN config\n");
		goto out;
	}

	(void) printf("%-20s%02x:%02x:%02x:%02x:%02x:%02x\n", "MAC Address:",
	    lancfg.ilc_macaddr[0], lancfg.ilc_macaddr[1],
	    lancfg.ilc_macaddr[2], lancfg.ilc_macaddr[3],
	    lancfg.ilc_macaddr[4], lancfg.ilc_macaddr[5]);

	(void) printf("%-20s%s\n", "VLAN Status:",
	    lancfg.ilc_vlan_enabled ? "Enabled" : "Disabled");

	if (lancfg.ilc_vlan_enabled == B_TRUE)
		(void) printf("%-20s%u\n", "VLAN ID", lancfg.ilc_vlan_id);

	if (lancfg.ilc_ipv4_enabled == B_TRUE && dump_ipv4_config(&lancfg) != 0)
		goto out;

	if (lancfg.ilc_ipv6_enabled == B_TRUE && dump_ipv6_config(&lancfg) != 0)
		goto out;

	status = 0;
out:
	ipmi_close(ihp);

	return (status);
}
