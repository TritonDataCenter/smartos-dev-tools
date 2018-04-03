/*
 * Copyright (c) 2018, Joyent, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <libipmi.h>
#include <libnvpair.h>
#include <fm/libtopo.h>
#include <string.h>
#include <sys/types.h>

/*
 * The largest possible SDR ID length is 2^5+1
 */
#define	MAX_ID_LEN	33

static const char *pname;
static const char optstr[] = "E:h:p:u:t:T:";

static void
usage()
{
	(void) fprintf(stderr, "usage: %s -t <bmc|lan> [-h host] [-u user] "
	    "[-p passwd] [-T SDR_Type] [-E entity_type]\n\n", pname);
}

#define ISBITSET(MASK, BIT)	((MASK & BIT) == BIT)

static void
dump_full_sensor(ipmi_handle_t *hdl, ipmi_sdr_full_sensor_t *fs)
{
	ipmi_sensor_reading_t *reading;
	ipmi_sensor_thresholds_t thresh = { 0 };
	double conv_reading;
	char buf[255], *notreadable = "Not Readable";
	uint8_t mask;
	int ret;

	ipmi_sensor_type_name(fs->is_fs_type, buf, sizeof (buf));
	(void) printf("%-35s0x%x (%s)\n", "Sensor Type", fs->is_fs_type, buf);

	ipmi_sensor_reading_name(fs->is_fs_type, fs->is_fs_reading_type, buf,
	    sizeof (buf));
	(void) printf("%-35s0x%x (%s)\n", "Reading Type",
	    fs->is_fs_reading_type, buf);

	if ((reading = ipmi_get_sensor_reading(hdl, fs->is_fs_number)) ==
	    NULL) {
		(void) fprintf(stderr, "Failed to get sensor reading (%s)\n",
		    ipmi_errmsg(hdl));
		return;
	}

	topo_sensor_state_name(fs->is_fs_type, reading->isr_state, buf,
	    sizeof (buf));
	if (fs->is_fs_reading_type != IPMI_RT_THRESHOLD) {
		(void) printf("%-35s0x%04x (%s)\n", "Discrete State",
		    reading->isr_state, buf);
		return;
	} else
		(void) printf("%-35s0x%02x (%s)\n", "Discrete State",
		    reading->isr_state, buf);

	if (ipmi_sdr_conv_reading(fs, reading->isr_reading, &conv_reading) !=
	    0) {
		(void) fprintf(stderr, "Failed to convert sensor reading "
		    "(%s)\n", ipmi_errmsg(hdl));
		return;
	}
	ipmi_sensor_units_name(fs->is_fs_unit2, buf, sizeof (buf));
	(void) printf("%-35s%.2lf %s\n", "Analog Reading", conv_reading, buf);

	if (ipmi_get_sensor_thresholds(hdl, &thresh, fs->is_fs_number) != 0) {
		(void) fprintf(stderr, "Failed to get sensor thresholds "
		    "(%s)\n", ipmi_errmsg(hdl));
		return;
	}

	mask = thresh.ithr_readable_mask;
	if (!ISBITSET(mask, IPMI_SENSOR_THRESHOLD_LOWER_NONCRIT) ||
	    ipmi_sdr_conv_reading(fs, thresh.ithr_lower_noncrit,
	    &conv_reading) != 0)
		(void) printf("%-35s%s\n", "Lower Non-Critical", notreadable);
	else
		(void) printf("%-35s%.2lf\n", "Lower Non-Critical",
		    conv_reading);

	if (!ISBITSET(mask, IPMI_SENSOR_THRESHOLD_LOWER_CRIT) ||
	    ipmi_sdr_conv_reading(fs, thresh.ithr_lower_crit,
	    &conv_reading) != 0)
		(void) printf("%-35s%s\n", "Lower Critical", notreadable);
	else
		(void) printf("%-35s%.2lf\n", "Lower Critical", conv_reading);

	if (!ISBITSET(mask, IPMI_SENSOR_THRESHOLD_LOWER_NONRECOV) ||
	    ipmi_sdr_conv_reading(fs, thresh.ithr_lower_nonrec,
	    &conv_reading) != 0)
		(void) printf("%-35s%s\n", "Lower Non-Recoverable",
		    notreadable);
	else
		(void) printf("%-35s%.2lf\n", "Lower Non-Recoverable",
		    conv_reading);

	if (!ISBITSET(mask, IPMI_SENSOR_THRESHOLD_UPPER_NONCRIT) ||
	    ipmi_sdr_conv_reading(fs, thresh.ithr_upper_noncrit,
	    &conv_reading) != 0)
		(void) printf("%-35s%s\n", "Upper Non-Critical", notreadable);
	else
		(void) printf("%-35s%.2lf\n", "Upper Non-Critical",
		    conv_reading);

	if (!ISBITSET(mask, IPMI_SENSOR_THRESHOLD_UPPER_CRIT) ||
	    ipmi_sdr_conv_reading(fs, thresh.ithr_upper_crit,
	    &conv_reading) != 0)
		(void) printf("%-35s%s\n", "Upper Critical", notreadable);
	else
		(void) printf("%-35s%.2lf\n", "Upper Critical", conv_reading);

	if (!ISBITSET(mask, IPMI_SENSOR_THRESHOLD_UPPER_NONRECOV) ||
	    ipmi_sdr_conv_reading(fs, thresh.ithr_upper_nonrec,
	    &conv_reading) != 0)
		(void) printf("%-35s%s\n", "Upper Non-Recoverable",
		    notreadable);
	else
		(void) printf("%-35s%.2lf\n", "Upper Non-Recoverable",
		    conv_reading);
}

static void
dump_compact_sensor(ipmi_handle_t *hdl, ipmi_sdr_compact_sensor_t *cs)
{
	ipmi_sensor_reading_t *reading;
	char buf[255];

	ipmi_sensor_type_name(cs->is_cs_type, buf, sizeof (buf));
	(void) printf("%-35s0x%x (%s)\n", "Sensor Type", cs->is_cs_type, buf);

	ipmi_sensor_reading_name(cs->is_cs_type, cs->is_cs_reading_type, buf,
	    sizeof (buf));
	(void) printf("%-35s0x%x (%s)\n", "Reading Type",
	    cs->is_cs_reading_type, buf);

	if ((reading = ipmi_get_sensor_reading(hdl, cs->is_cs_number)) ==
	    NULL) {
		(void) fprintf(stderr, "Failed to get sensor reading (%s)\n",
		    ipmi_errmsg(hdl));
		return;
	}
	topo_sensor_state_name(cs->is_cs_type, reading->isr_state, buf,
	    sizeof (buf));
	(void) printf("%-35s0x%04x (%s)\n", "Discrete State",
	    reading->isr_state, buf);
}

static int
dump_fru_rec(ipmi_handle_t *hdl, ipmi_sdr_fru_locator_t *fl)
{
	char *frubuf;
	ipmi_fru_prod_info_t prodinfo = { 0 };
	ipmi_fru_brd_info_t boardinfo = { 0 };

	if (ipmi_fru_read(hdl, fl, &frubuf) < 0)
		return (-1);

	if (ipmi_fru_parse_product(hdl, frubuf, &prodinfo) == 0) {
		(void) printf("%-35s%s\n", "Product Manufacturer",
		    prodinfo.ifpi_manuf_name);
		(void) printf("%-35s%s\n", "Product Name",
		    prodinfo.ifpi_product_name);
		(void) printf("%-35s%s\n", "Product P/N",
		    prodinfo.ifpi_part_number);
		(void) printf("%-35s%s\n", "Product Version",
		    prodinfo.ifpi_product_version);
		(void) printf("%-35s%s\n", "Product S/N",
		    prodinfo.ifpi_product_serial);
		(void) printf("%-35s%s\n", "Product Asset Tag",
		    prodinfo.ifpi_asset_tag);
	}
	if (ipmi_fru_parse_board(hdl, frubuf, &boardinfo) == 0) {
		(void) printf("%-35s%s\n", "Board Manufacturer",
		    boardinfo.ifbi_manuf_name);
		(void) printf("%-35s%s\n", "Board Name",
		    boardinfo.ifbi_board_name);
		(void) printf("%-35s%s\n", "Board P/N",
		    boardinfo.ifbi_part_number);
		(void) printf("%-35s%s\n", "Board S/N",
		    boardinfo.ifbi_product_serial);
	}

	free(frubuf);
	return (0);
}

static void
dump_ea_rec(ipmi_handle_t *hdl, ipmi_sdr_entity_association_t *ea)
{
	char buf[255];

	for (int i = 0; i < 4; i++) {
		if (ea->is_ea_sub[i].is_ea_sub_id == 0) {
			(void) printf("\t%-30s: %s\n", "Contained Entity ID",
			    "Unspecified");
			(void) printf("\t%-30s: %s\n", "Contained Entity Inst",
			    "Unspecified");
		} else {
			ipmi_entity_name(ea->is_ea_sub[i].is_ea_sub_id, buf,
			    sizeof (buf));
			(void) printf("\t%-30s: %u (%s)\n",
			    "Contained Entity ID",
			    ea->is_ea_sub[i].is_ea_sub_id, buf);
			(void) printf("\t%-30s: %u\n", "Contained Entity Inst",
			    ea->is_ea_sub[i].is_ea_sub_instance);
		}
	}
}

struct cbarg {
	long cb_sdr_type;
	long cb_entity_id;
};

static int
dump_rec(ipmi_handle_t *hdl, const char *id, ipmi_sdr_t *sdr, void *data)
{
	ipmi_sdr_full_sensor_t *fs;
	ipmi_sdr_compact_sensor_t *cs;
	ipmi_sdr_event_only_t *eo;
	ipmi_sdr_fru_locator_t *fl;
	ipmi_sdr_generic_locator_t *gl;
	ipmi_sdr_entity_association_t *ea;
	char sdr_id[MAX_ID_LEN], buf[255];
	struct cbarg *arg = (struct cbarg *)data;

	/*
	 * If we're filtering on SDR type and the type doesn't match, bail out.
	 */
	if (arg->cb_sdr_type != 0 && arg->cb_sdr_type != sdr->is_type)
		return (0);

	switch (sdr->is_type) {
	case IPMI_SDR_TYPE_FULL_SENSOR:
		fs = (ipmi_sdr_full_sensor_t *)sdr->is_record;
		if (arg->cb_entity_id != 0 && arg->cb_entity_id !=
		    fs->is_fs_entity_id)
			return (0);
		(void) strlcpy(sdr_id, fs->is_fs_idstring, fs->is_fs_idlen + 1);
		(void) printf("\n0x%x\tFull Sensor\n", sdr->is_id);
		(void) printf("%-35s%s\n", "Entity Name", sdr_id);
		ipmi_entity_name(fs->is_fs_entity_id, buf, sizeof (buf));
		(void) printf("%-35s%u (%s)\n", "Entity ID",
		    fs->is_fs_entity_id, buf);
		(void) printf("%-35s%u\n", "Entity Instance",
		    fs->is_fs_entity_instance);
		dump_full_sensor(hdl, fs);
		break;
	case IPMI_SDR_TYPE_COMPACT_SENSOR:
		cs = (ipmi_sdr_compact_sensor_t *)sdr->is_record;
		if (arg->cb_entity_id != 0 && arg->cb_entity_id !=
		    cs->is_cs_entity_id)
			return (0);
		(void) strlcpy(sdr_id, cs->is_cs_idstring, cs->is_cs_idlen + 1);
		(void) printf("\n0x%x\tCompact Sensor\n", sdr->is_id);
		(void) printf("%-35s%s\n", "Entity Name", sdr_id);
		ipmi_entity_name(cs->is_cs_entity_id, buf, sizeof (buf));
		(void) printf("%-35s%u (%s)\n", "Entity ID",
		    cs->is_cs_entity_id, buf);
		(void) printf("%-35s%u\n", "Entity Instance",
		    cs->is_cs_entity_instance);
		dump_compact_sensor(hdl, cs);
		break;
	case IPMI_SDR_TYPE_EVENT_ONLY:
		eo = (ipmi_sdr_event_only_t *)sdr->is_record;
		if (arg->cb_entity_id != 0 && arg->cb_entity_id !=
		    eo->is_eo_entity_id)
			return (0);
		(void) strlcpy(sdr_id, eo->is_eo_idstring, eo->is_eo_idlen + 1);
		(void) printf("\n0x%x\tEvent-Only\n", sdr->is_id);
		(void) printf("%-35s%s\n", "Entity Name", sdr_id);
		ipmi_entity_name(eo->is_eo_entity_id, buf, sizeof (buf));
		(void) printf("%-35s%u (%s)\n", "Entity ID",
		    eo->is_eo_entity_id, buf);
		(void) printf("%-35s%u\n", "Entity Instance",
		    eo->is_eo_entity_instance);
		ipmi_sensor_type_name(eo->is_eo_sensor_type, buf,
		    sizeof (buf));
		(void) printf("%-35s0x%x (%s)\n", "Sensor Type",
		    eo->is_eo_sensor_type, buf);
		ipmi_sensor_reading_name(eo->is_eo_reading_type,
		    eo->is_eo_reading_type, buf, sizeof (buf));
		(void) printf("%-35s0x%x (%s)\n", "Reading Type",
		    eo->is_eo_reading_type, buf);
		break;
	case IPMI_SDR_TYPE_FRU_LOCATOR:
		fl = (ipmi_sdr_fru_locator_t *)sdr->is_record;
		if (arg->cb_entity_id != 0 && arg->cb_entity_id !=
		    fl->is_fl_entity)
			return (0);
		(void) strlcpy(sdr_id, fl->is_fl_idstring, fl->is_fl_idlen + 1);
		(void) printf("\n0x%x\tFRU Locator\n", sdr->is_id);
		(void) printf("%-35s%s\n", "Entity Name", sdr_id);
		ipmi_entity_name(fl->is_fl_entity, buf, sizeof (buf));
		(void) printf("%-35s%u (%s)\n", "Entity ID", fl->is_fl_entity,
		    buf);
		(void) printf("%-35s%u\n", "Entity Instance",
		    fl->is_fl_instance);
		if (dump_fru_rec(hdl, fl) != 0)
			(void) printf("failed to read FRU inventory area\n");
		break;
	case IPMI_SDR_TYPE_GENERIC_LOCATOR:
		gl = (ipmi_sdr_generic_locator_t *)sdr->is_record;
		if (arg->cb_entity_id != 0 && arg->cb_entity_id !=
		    gl->is_gl_entity)
			return (0);
		(void) strlcpy(sdr_id, gl->is_gl_idstring, gl->is_gl_idlen + 1);
		(void) printf("\n0x%x\tGeneric Device Locator\n", sdr->is_id);
		(void) printf("%-35s%s\n", "Entity Name", sdr_id);
		ipmi_entity_name(gl->is_gl_entity, buf, sizeof (buf));
		(void) printf("%-35s%u (%s)\n", "Entity ID", gl->is_gl_entity,
		    buf);
		(void) printf("%-35s%u\n", "Entity Instance",
		    gl->is_gl_instance);
		break;
	case IPMI_SDR_TYPE_ENTITY_ASSOCIATION:
		ea = (ipmi_sdr_entity_association_t *)sdr->is_record;
		if (arg->cb_entity_id != 0 && arg->cb_entity_id !=
		    ea->is_ea_entity_id)
			return (0);
		(void) printf("\n0x%x\tEntity Association\n", sdr->is_id);
		(void) printf("%-35s%s\n", "Entity Name", "N/A");
		ipmi_entity_name(ea->is_ea_entity_id, buf, sizeof (buf));
		(void) printf("%-35s%u (%s)\n", "Entity ID",
		    ea->is_ea_entity_id, buf);
		(void) printf("%-35s%u\n", "Entity Instance",
		    ea->is_ea_entity_instance);
		dump_ea_rec(hdl, ea);
		break;
	default:
		(void) printf("\nUnrecognized record type: 0x%u\n",
		    sdr->is_type);
	}

	return (0);
}

int
main(int argc, char **argv)
{
	ipmi_handle_t *ihp;
	char *errmsg;
	uint_t xport_type = IPMI_TRANSPORT_BMC;
	char c, *host = NULL, *user = NULL, *passwd = NULL;
	int err, status = 1;
	long sdr_type, ent_id;
	struct cbarg arg = { 0 };
	nvlist_t *params = NULL;

	pname = argv[0];
	while (optind < argc) {
		while ((c = getopt(argc, argv, optstr)) != -1) {
			switch (c) {
			case 'E':
				if ((ent_id = strtol(optarg, NULL, 0)) !=
				    0 && ent_id < 0xFF) {
					arg.cb_entity_id = ent_id;
				} else {
					(void) fprintf(stderr,
					    "ABORT: invalid SDR type\n");
					usage();
					return (2);
				}
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
					return (2);
				}
				break;
			case 'T':
				if ((sdr_type = strtol(optarg, NULL, 0)) !=
				    0) {
					arg.cb_sdr_type = sdr_type;
				} else {
					(void) fprintf(stderr,
					    "ABORT: invalid SDR type\n");
					usage();
					return (2);
				}
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

	if (ipmi_sdr_iter(ihp, dump_rec, &arg) != 0) {
		(void) fprintf(stderr, "failed to walk sdr\n");
		goto out;
	}
	status = 0;
out:
	ipmi_close(ihp);

	return (status);
}
