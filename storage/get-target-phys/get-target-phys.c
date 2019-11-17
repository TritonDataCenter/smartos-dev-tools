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
#include <string.h>
#include <unistd.h>

#include <sys/byteorder.h>
#include <sys/sysmacros.h>
#include <sys/varargs.h>
#include <sys/scsi/scsi.h>

#define	EXIT_USAGE	2

#define PROTOCOL_SPECIFIC_PAGE		0x18
#define ENHANCED_PHY_CONTROL_PAGE	0x19

#pragma pack(1)

/*
 * Mode Parameter Header
 * 
 * See SPC 4, sectin 7.5.5
 */
typedef struct scsi_mode10_hdr {
	uint16_t smh10_len;
	uint8_t smh10_medtype;
	uint8_t smh10_devparm;
	uint16_t _reserved;
	uint16_t smh10_bdlen;
} scsi_mode10_hdr_t;

/*
 * SCSI MODE SENSE (10) command
 * 
 * See SPC 4, section 6.14
 */
typedef struct scsi_modesense_10_cmd {
	uint8_t scm10_opcode;
	DECL_BITFIELD4(
		_reserved1	:3,
		scm10_dbd	:1,
		scm10_llbaa	:1,
		_reserved2	:3);
	DECL_BITFIELD2(
		scm10_pagecode	:6,
		scm10_pc	:2);
	uint8_t scm10_subpagecode;
	uint8_t _reserved3[3];
	uint16_t scm10_buflen;
	uint8_t scm10_control;
} scsi_modesense_10_cmd_t;

/*
 * See SPL 4, section 9.2.7.5
 */
typedef struct sas_phys_disc_mode_page {
	DECL_BITFIELD3(
	    spdm_pagecode	:6,
	    spdm_spf		:1,
	    spdm_ps		:1);
	uint8_t spdm_subpagecode;
	uint16_t spdm_pagelen;
	uint8_t _reserved1;
	DECL_BITFIELD2(
	    spdm_proto_id	:4,
	    _reserved2		:4);
	uint8_t spdm_gencode;
	uint8_t spdm_nphys;
	uint8_t spdm_descr[1];
} sas_phys_disc_mode_page_t;

/*
 * SAS Mode PHY Descriptor
 * 
 * See SPL 4, section 9.2.7.5
 */
typedef struct sas_phy_descriptor {
	uint8_t _reserved1;
	uint8_t spde_phy_id;
	uint16_t _reserved2;
	DECL_BITFIELD3(
	    spde_attach_reason	:4,
	    spde_attach_devtype	:3,
	    _reserved3		:1);
	DECL_BITFIELD2(
	    spde_neg_rate	:4,
	    spde_reason		:4);
	DECL_BITFIELD5(
	    _reserved4			:1,
	    spde_att_smp_ini_port	:1,
	    spde_att_stp_ini_port	:1,
	    spde_att_ssp_ini_port	:1,
	    _reserved5			:4);
	DECL_BITFIELD5(
	    _reserved6			:1,
	    spde_att_smp_tgt_port	:1,
	    spde_att_stp_tgt_port	:1,
	    spde_att_ssp_tgt_port	:1,
	    _reserved7			:4);
	uint64_t spde_sas_addr;
	uint64_t spde_att_sas_addr;
	uint8_t spde_att_phy_id;
	DECL_BITFIELD7(
		spde_att_brk_reply	:1,
		spde_att_rqst_imside	:1,
		spde_att_inside	:1,
		spde_att_partial_cap	:1,
		spde_att_slumber_cap	:1,
		spde_att_power_cap	:2,
		spde_att_persist_cap	:1);
	DECL_BITFIELD4(
		spde_att_pwr_dis_cap	:1,
		spde_att_smp_prio_cpa	:1,
		spde_att_apta_cap	:1,
		_reserved8		:5);
	uint8_t _reserved9[5];
	DECL_BITFIELD2(
		spde_hw_min_rate	:4,
		spde_prog_min_rate	:4);
	DECL_BITFIELD2(
		spde_hw_max_rate	:4,
		spde_prog_max_rate	:4);
	uint8_t _reserved10[8];
	uint8_t _spde_vendor_spec[2];
	uint8_t _reserved11[4];
} sas_phy_descriptor_t;

#define	PHY_LINK_RATE_END	0x0D

static const char *link_rate_strings[] = {
    "PHY enabled, unknown link rate",
    "PHY disabled",
    "PHY reset problem",
    "PHY enabled, spin hold",
    "PHY enabled, SATA link rate still negotiating",
    "PHY reset in progress",
    "PHY enabled, unsupported device attached",
    "0x07, Reserved",
    "PHY enabled, 1.5 Gbits/s",
    "PHY enabled, 3.0 Gbits/s",
    "PHY enabled, 6.0 GBits/s",
    "PHY enabled, 12.0 GBits/s",
    "PHY enabled, 22.5 GBits/s"
};

/*
 * See SPL 4, section 9.2.8.1
 */
typedef struct sas_log_page {
	DECL_BITFIELD3(
	    slp_pagecode	:6,
	    slp_spf		:1,
	    slp_ds		:1);
	uint8_t slp_subpagecode;
	uint16_t slp_pagelen;
	uint8_t slp_portparam[1];
} sas_log_page_t;

/*
 * Protocol Specific Port log parameter
 * 
 * See SPL 4. section 9.2.8.2
 */
typedef struct sas_port_param {
	uint16_t spp_port_id;
	DECL_BITFIELD5(
	    spp_fml		:2,
	    _reserved1		:3,
	    spp_tsd		:1,
	    _reserved2		:1,
	    spp_du		:1);
	uint8_t spp_param_len;
	DECL_BITFIELD2(
	    spp_proto_id	:4,
	    _reserved3		:4);
	uint8_t _reserved4;
	uint8_t spp_gencode;
	uint8_t spp_nphys;
	uint8_t spp_descr[1];
} sas_port_param_t;

/*
 * SAS PHY Log Descriptor
 * 
 * See SPL 4, section 9.2.8.2
 */
typedef struct sas_phy_log_descr {
	uint8_t _reserved1;
	uint8_t sld_phy_id;
	uint8_t _reserved2;
	uint8_t sld_len;
	DECL_BITFIELD3(
	    sld_att_reason		:4,
	    sld_att_devtype		:3,
	    _reserved			:1);
	DECL_BITFIELD2(
	    sld_neg_rate		:4,
	    sld_neg_reason		:4);
	DECL_BITFIELD5(
	    _reserved5:1,
	    sld_att_smp_ini_port	:1,
	    sld_att_stp_ini_port	:1,
	    sld_att_ssp_ini_port	:1,
	    _reserved6			:4);
	DECL_BITFIELD5(
	    _reserved7			:1,
	    sld_att_smp_tgt_port	:1,
	    sld_att_stp_tgt_port	:1,
	    sld_att_ssp_tgt_port	:1,
	    _reserved8			:4);
	uint64_t sld_sas_addr;
	uint64_t sld_att_sas_addr;
	uint8_t sld_att_phy_id;
	uint8_t _reserved9[7];

	/* PHY error counters */
	uint32_t sld_inv_dword;
	uint32_t sld_running_disp;
	uint32_t sld_loss_sync;
	uint32_t sld_reset_prob;

	uint8_t _reserved10[2];
	uint8_t sld_event_descr_len;
	uint8_t sld_num_event_descr;
	uint8_t sld_event_descr[1];
} sas_phy_log_descr_t;

#pragma pack()

static const char *pname;
static const char optstr[] = "dhrt:";
static boolean_t do_debug = B_FALSE;

static void
usage()
{
	(void) fprintf(stderr, "usage: %s -t <target devpath> [-d]\n", pname);
}

void
debug(const char *format, ...)
{
	if (do_debug) {
		va_list ap;

		va_start(ap, format);
		(void) vfprintf(stdout, format, ap);
		va_end(ap);
		(void) fprintf(stdout, "\n");
		(void) fflush(stdout);
	}
}

static void dump_raw(uint8_t *buf, uint16_t len)
{
	uint_t i = 0, j = 0;

	(void) printf("%02x    ", i);
	while (i < len) {
		(void) printf("%02x ", buf[i++]);
		if (j == 7)
			(void) printf("  ");
		if (j == 15) {
			(void) printf("\n");
			(void) printf("%02x    ", i);
			j = 0;
		} else
			j++;
	}
	printf("\n");
}

static int
scsi_mode_sense(int fd, uint8_t pagecode, uint8_t subpagecode,
    uchar_t *pagebuf, uint16_t buflen)
{
	struct uscsi_cmd ucmd_buf;
	scsi_modesense_10_cmd_t cdb;
	struct scsi_extended_sense sense_buf;

	memset((void *)pagebuf, 0, buflen);
	memset((void *)&cdb, 0, sizeof (scsi_modesense_10_cmd_t));
	memset((void *)&ucmd_buf, 0, sizeof (ucmd_buf));
	memset((void *)&sense_buf, 0, sizeof (sense_buf));

	cdb.scm10_opcode = SCMD_MODE_SENSE_G1;
	cdb.scm10_pagecode = pagecode;
	cdb.scm10_subpagecode = subpagecode;
	cdb.scm10_buflen = BE_16(buflen);

	debug("Reading mode page 0x%x, subpage: 0x%x\n", pagecode,
	    subpagecode);
	if (do_debug) {
		(void) printf("CDB:\n");
		dump_raw((uint8_t *)&cdb, sizeof (scsi_modesense_10_cmd_t));
		(void) printf("\n");
	}

	ucmd_buf.uscsi_cdb = (uint8_t *)&cdb;
	ucmd_buf.uscsi_cdblen = sizeof (scsi_modesense_10_cmd_t);
	ucmd_buf.uscsi_bufaddr = (caddr_t)pagebuf;
	ucmd_buf.uscsi_buflen = buflen;
	ucmd_buf.uscsi_rqbuf = (caddr_t)&sense_buf;
	ucmd_buf.uscsi_rqlen = sizeof (struct scsi_extended_sense);
	ucmd_buf.uscsi_flags = USCSI_RQENABLE | USCSI_READ | USCSI_SILENT;
	ucmd_buf.uscsi_timeout = 60;

	if (ioctl(fd, USCSICMD, &ucmd_buf) < 0) {
		(void) fprintf(stderr, "failed to read mode page (%s)\n",
		    strerror(errno));
		return (-1);
	}
	return (0);
}

static int
scsi_log_sense(int fd, uint8_t pagecode, uchar_t *pagebuf, uint16_t pagelen)
{
	struct uscsi_cmd ucmd_buf;
	uchar_t	cdb_buf[CDB_GROUP1];
	struct scsi_extended_sense sense_buf;

	memset((void *)pagebuf, 0, pagelen);
	memset((void *)&cdb_buf, 0, sizeof (cdb_buf));
	memset((void *)&ucmd_buf, 0, sizeof (ucmd_buf));
	memset((void *)&sense_buf, 0, sizeof (sense_buf));

	cdb_buf[0] = SCMD_LOG_SENSE_G1;
	cdb_buf[2] = (0x01 << 6) | pagecode;
	cdb_buf[7] = (uchar_t)((pagelen & 0xFF00) >> 8);
	cdb_buf[8] = (uchar_t)(pagelen  & 0x00FF);

	debug("Reading log page 0x%x\n", pagecode);
	if (do_debug) {
		(void) printf("CDB:\n");
		dump_raw(cdb_buf, CDB_GROUP1);
		(void) printf("\n");
	}
	ucmd_buf.uscsi_cdb = (char *)cdb_buf;
	ucmd_buf.uscsi_cdblen = sizeof (cdb_buf);
	ucmd_buf.uscsi_bufaddr = (caddr_t)pagebuf;
	ucmd_buf.uscsi_buflen = pagelen;
	ucmd_buf.uscsi_rqbuf = (caddr_t)&sense_buf;
	ucmd_buf.uscsi_rqlen = sizeof (struct scsi_extended_sense);
	ucmd_buf.uscsi_flags = USCSI_RQENABLE | USCSI_READ | USCSI_SILENT;
	ucmd_buf.uscsi_timeout = 60;

	if (ioctl(fd, USCSICMD, &ucmd_buf) < 0) {
		(void) fprintf(stderr, "failed to read log page (%s)\n",
		    strerror(errno));
		return (-1);
	}
	return (0);
}

static void
dump_phy_descr(sas_phy_descriptor_t *phy_desc, sas_phy_log_descr_t *port_desc)
{
	(void) printf("\nPHY ID: 0x%x\n", phy_desc->spde_phy_id);
	(void) printf("%-30s %" PRIx64 "\n", "  SAS Address:",
	    BE_64(phy_desc->spde_sas_addr));
	(void) printf("%-30s %" PRIx64 "\n", "  Atached SAS Address:",
	    BE_64(phy_desc->spde_att_sas_addr));
	(void) printf("%-30s 0x%x\n", "  Attached PHY ID:",
	    phy_desc->spde_att_phy_id);
	(void) printf("%-30s %s\n", "  Negotiated Link Rate:",
	    link_rate_strings[phy_desc->spde_neg_rate]);
	if (port_desc != NULL) {
		(void) printf("%-30s %u\n", "  Invalid DWORD Count:",
		    BE_32(port_desc->sld_inv_dword));
		(void) printf("%-30s %u\n", "  Running Disparity Count:",
		    BE_32(port_desc->sld_running_disp));
		(void) printf("%-30s %u\n", "  Loss DWORD Sync Count:",
		    BE_32(port_desc->sld_loss_sync));
		(void) printf("%-30s %u\n", "  PHY Reset Problem Count:",
		    BE_32(port_desc->sld_reset_prob));
	}
}

int
main(int argc, char *argv[])
{
	char c, *target = NULL;
	int fd, status = EXIT_FAILURE;
	uchar_t modepagebuf[4096], logpagebuf[4096];
	uint16_t buflen = 4096, pagelen, bdlen;
	scsi_mode10_hdr_t *modehdr;
	void *blkdescr;
	sas_phys_disc_mode_page_t *modepage;
	sas_log_page_t *logpage;
	sas_port_param_t *pparam;

	pname = argv[0];

	while (optind < argc) {
		while ((c = getopt(argc, argv, optstr)) != -1) {
			switch (c) {
			case 'd':
				do_debug = B_TRUE;
				break;
			case 'h':
				usage();
				return (EXIT_USAGE);
			case 't':
				target = optarg;
				break;
			default:
				usage();
				return (EXIT_USAGE);
			}
		}
	}
	if (target == NULL) {
		(void) fprintf(stderr, "-t is a required argument\n");
		usage();
		return (EXIT_SUCCESS);
	}

	if ((fd = open(target, O_RDWR |O_NONBLOCK)) < 0) {
		(void) fprintf(stderr, "failed to open %s (%s)\n", target,
		    strerror(errno));
		return (EXIT_FAILURE);
	}

	if (scsi_mode_sense(fd, ENHANCED_PHY_CONTROL_PAGE, 0x1, modepagebuf,
	    buflen) != 0) {
    		goto done;
	}

	modehdr = (scsi_mode10_hdr_t *)modepagebuf;
	blkdescr = modepagebuf + sizeof (scsi_mode10_hdr_t);
	bdlen = BE_16(modehdr->smh10_bdlen);

	if (do_debug) {
		(void) printf("Mode Header:\n");
		dump_raw((uint8_t *)modehdr, sizeof (scsi_mode10_hdr_t));
		(void) printf("Block Descriptor:\n");
		dump_raw((uint8_t *)blkdescr, bdlen);
	}

	modepage = (sas_phys_disc_mode_page_t *)
	    (modepagebuf + sizeof (scsi_mode10_hdr_t) + bdlen);
	pagelen = BE_16(modepage->spdm_pagelen);
	if (do_debug) {
		(void) printf("Mode Page:\n");
		dump_raw((uint8_t *)modepage, pagelen + 4);
	}
	debug("Page Code: 0x%x", modepage->spdm_pagecode);
	debug("Subpage Code: 0x%x", modepage->spdm_subpagecode);
	debug("Page Length: 0x%x (%u) bytes", pagelen, pagelen);

	if (scsi_log_sense(fd, PROTOCOL_SPECIFIC_PAGE, logpagebuf,
	    buflen) != 0)
    		goto done;

	logpage = (sas_log_page_t *)logpagebuf;
	pagelen = BE_16(logpage->slp_pagelen);
	if (do_debug) {
		(void) printf("Response:\n");
		dump_raw((uint8_t *)logpage, pagelen + 4);
	}
	debug("Page Code: 0x%x", logpage->slp_pagecode);
	debug("Subpage Code: 0x%x", logpage->slp_subpagecode);
	debug("Page Length: 0x%x (%u) bytes\n", pagelen, pagelen);

	(void) printf("Num Target PHYS: %u\n", modepage->spdm_nphys);

	/*
	 * Because we're only dealing with SAS targets, we make the following
	 * assumptions here:
	 * 1. All ports on the target are narrow ports (one PHY wide)
	 * 2. If a PHY is unattached, we don't care about the associated port.
	 */
	for (uint_t phy = 0; phy < modepage->spdm_nphys; phy++) {
		sas_phy_log_descr_t *port_descr;
		sas_phy_descriptor_t *phy_descr = (sas_phy_descriptor_t *) 
		    (modepage->spdm_descr + (sizeof (sas_phy_descriptor_t) * phy));

		if (phy_descr->spde_att_sas_addr == 0)
			port_descr = NULL;
		else {
			pparam = (sas_port_param_t *)(logpage->slp_portparam + 
			    (sizeof (sas_port_param_t) * phy));

			port_descr = (sas_phy_log_descr_t *) pparam->spp_descr;
		}
 		dump_phy_descr(phy_descr, port_descr);
	}
	status = EXIT_SUCCESS;

done:
    (void) close(fd);
    return (status);
}
