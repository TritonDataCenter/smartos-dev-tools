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
#include <sys/types.h>
#include <sys/stat.h>

/*
 * These headers aren't delivered, so we'll have to build against an ON source
 * tree.  See the comments in the makefile.
 */
#include <fmd_ckpt.h>
#include <fmd_module.h>

typedef struct fmd_ckpt_strtab {
	char *ckp_strp;		/* string table pointer */
	size_t ckp_strsz;	/* string table size */
} fmd_ckpt_strtab_t;

static fmd_ckpt_strtab_t strtab = { 0 };

/*
 * adapted from fcf_hdr() in usr/src/cmd/fm/fmd/common/fmd_mdb.c
 */
static void
dump_fcf_hdr(fcf_hdr_t *h)
{
	(void) printf("==========\n");
	(void) printf("FCF Header\n");
	(void) printf("==========\n");
        (void) printf("fcfh_ident.id_magic = 0x%x, %c, %c, %c\n",
            h->fcfh_ident[FCF_ID_MAG0], h->fcfh_ident[FCF_ID_MAG1],
            h->fcfh_ident[FCF_ID_MAG2], h->fcfh_ident[FCF_ID_MAG3]);

	switch (h->fcfh_ident[FCF_ID_MODEL]) {
	case FCF_MODEL_ILP32:
                (void) printf("fcfh_ident.id_model = ILP32\n");
                break;
	case FCF_MODEL_LP64:
                (void) printf("fcfh_ident.id_model = LP64\n");
                break;
	default:
                (void) printf("fcfh_ident.id_model = 0x%x\n",
                    h->fcfh_ident[FCF_ID_MODEL]);
	}

	switch (h->fcfh_ident[FCF_ID_ENCODING]) {
	case FCF_ENCODE_LSB:
                (void) printf("fcfh_ident.id_encoding = LSB\n");
                break;
	case FCF_ENCODE_MSB:
                (void) printf("fcfh_ident.id_encoding = MSB\n");
                break;
	default:
                (void) printf("fcfh_ident.id_encoding = 0x%x\n",
                    h->fcfh_ident[FCF_ID_ENCODING]);
	}

	(void) printf("fcfh_ident.id_version = %u\n",
            h->fcfh_ident[FCF_ID_VERSION]);

	(void) printf("fcfh_flags = 0x%x\n", h->fcfh_flags);
	(void) printf("fcfh_hdrsize = %u bytes\n", h->fcfh_hdrsize);
	(void) printf("fcfh_secsize = %u bytes\n", h->fcfh_secsize);
	(void) printf("fcfh_secnum = %u\n", h->fcfh_secnum);
	(void) printf("fcfh_secoff = %llu bytes\n", h->fcfh_secoff);
	(void) printf("fcfh_filesz = %llu bytes\n", h->fcfh_filesz);
	(void) printf("fcfh_cgen = %llu\n\n", h->fcfh_cgen);
}

static const char *
getstr(fcf_stridx_t sid)
{
        return (sid < strtab.ckp_strsz ? strtab.ckp_strp + sid : "<CORRUPT>");
}

static void
dump_sec_case(fcf_case_t *fmcase)
{
	(void) printf("fcfc_uuid = %s (0x%x)\n",
	    getstr(fmcase->fcfc_uuid), fmcase->fcfc_uuid);
	(void) printf("fcfc_state = %u\n", fmcase->fcfc_state);
	(void) printf("fcfc_bufs = %u\n", fmcase->fcfc_bufs);
	(void) printf("fcfc_events = %u\n", fmcase->fcfc_events);
	(void) printf("fcfc_suspects = %u\n\n", fmcase->fcfc_suspects);
}

static void
dump_sec_module(fcf_module_t *fmmod)
{
	(void) printf("fcfm_name = %s (0x%x)\n",
	    getstr(fmmod->fcfm_name), fmmod->fcfm_name);
	(void) printf("fcfm_path = %s (0x%x)\n",
	    getstr(fmmod->fcfm_path), fmmod->fcfm_path);
	(void) printf("fcfm_desc = %s (0x%x)\n",
	    getstr(fmmod->fcfm_desc), fmmod->fcfm_desc);	
	(void) printf("fcfm_vers = %s (0x%x)\n\n",
	    getstr(fmmod->fcfm_vers), fmmod->fcfm_vers);
}

static void
dump_sec_hdr(fcf_sec_t *s, char *ckpt_buf)
{
	fcf_case_t *fmcase;
	fcf_module_t *fmmod;

        static const char *const types[] = {
                "none",         /* FCF_SECT_NONE */
                "strtab",       /* FCF_SECT_STRTAB */
                "module",       /* FCF_SECT_MODULE */
                "case",         /* FCF_SECT_CASE */
                "bufs",         /* FCF_SECT_BUFS */
                "buffer",       /* FCF_SECT_BUFFER */
                "serd",         /* FCF_SECT_SERD */
                "events",       /* FCF_SECT_EVENTS */
                "nvlists",      /* FCF_SECT_NVLISTS */
        };

	(void) printf("%-10s %-5s %-5s %-5s %-6s %-5s\n",
	    "TYPE", "ALIGN", "FLAGS", "ENTSZ", "OFF", "SIZE");

	if (s->fcfs_type < sizeof (types) / sizeof (types[0]))
		(void) printf("%-10s ", types[s->fcfs_type]);
	else
		(void) printf("%-10u ", s->fcfs_type);

	(void) printf("%-5u %-#5x %-#5x %-6llx %-#5llx\n\n", s->fcfs_align,
	    s->fcfs_flags, s->fcfs_entsize, s->fcfs_offset, s->fcfs_size);

	switch (s->fcfs_type) {
	case (FCF_SECT_NONE):
		break;
	case (FCF_SECT_CASE):
		fmcase = (fcf_case_t *)(ckpt_buf + s->fcfs_offset);
		dump_sec_case(fmcase);
		break;
	case (FCF_SECT_MODULE):
		fmmod = (fcf_module_t *)(ckpt_buf + s->fcfs_offset);
		dump_sec_module(fmmod);
		break;
	case (FCF_SECT_STRTAB):
	case (FCF_SECT_BUFS):
	case (FCF_SECT_BUFFER):
	case (FCF_SECT_SERD):
	case (FCF_SECT_EVENTS):
	case (FCF_SECT_NVLISTS):
		break;
	default:
		(void) fprintf(stderr, "Unrecognized section type: %u\n",
		    s->fcfs_type);
	}
}

int
main(int argc, char **argv)
{
	int fd, i;
	struct stat64 st;
	char *ckbuf = NULL;
	fcf_hdr_t *hdr;
	fcf_sec_t *sec;

	if (argc != 2) {
		(void) fprintf(stderr, "\nUsage: %s <ckpt file>\n\n",
		    argv[0]);
		exit (2);
	}

	if ((fd = open(argv[1], O_RDONLY)) < 0 || fstat64(fd, &st) < 0) {
		(void) fprintf(stderr, "Failed to open checkpoint file: %s "
		    "(%s)\n", argv[1], strerror(errno));
	}

	if (! S_ISREG(st.st_mode)) {
		(void) fprintf(stderr, "ABORT: not a regular file\n");
		goto out;
	}

	if ((ckbuf = calloc(st.st_size, 1)) == NULL) {
		(void) fprintf(stderr, "failed to allocate %u byte buffer\n",
		    st.st_size);
		goto out;
	}

	(void) read(fd, ckbuf, st.st_size); 

	hdr = (fcf_hdr_t *)ckbuf;

	/*
	 * First as a sanity checl, verify the magic number matches what we'd
	 * expect for an fmd checkpoint file.
	 */
	if (bcmp(hdr->fcfh_ident, FCF_MAG_STRING, FCF_MAG_STRLEN) != 0) {
		(void) fprintf(stderr, "ABORT: not an fmd checkpoint file\n");
		goto out;
	}
	
	dump_fcf_hdr(hdr);

	/*
	 * Iterate through the section headers to find a pointer to the
	 * string table section.
	 */
        for (i = 0; i < hdr->fcfh_secnum; i++) {
                sec = (void *)hdr + hdr->fcfh_secoff + hdr->fcfh_secsize * i;
		if (sec->fcfs_type != FCF_SECT_STRTAB)
			continue;

		strtab.ckp_strp = ckbuf + sec->fcfs_offset;
		strtab.ckp_strsz = sec->fcfs_size;
		break;
        }

	for (i = 0; i < hdr->fcfh_secnum; i++) {
		sec = (void *)hdr + hdr->fcfh_secoff + hdr->fcfh_secsize * i;
		(void) printf("=============\n");
		(void) printf("SECTION %u (addr: %lx)\n", i, sec);
		(void) printf("=============\n");
		dump_sec_hdr(sec, ckbuf);
	}

out:
	free(ckbuf);
	(void) close(fd);

	return (0);
}

