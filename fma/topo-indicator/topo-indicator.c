/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2018, Joyent, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <fnmatch.h>
#include <string.h>
#include <fm/libtopo.h>
#include <fm/topo_list.h>

static const char *pname;
static const char optstr[] = "m:R:t:";

static void
usage()
{
	(void) fprintf(stderr, "usage: %s [-R root] -m <get|on|off> "
	    "-t <locate|service|ok2rm> <FMRI>\n\n", pname);
}

struct led_cb_arg {
	char *lcb_fmri;
	topo_led_type_t lcb_ledtype;
	char *lcb_ledtypestr;
	int lcb_ledmode;
};

static int
ledcb(topo_hdl_t *thp, tnode_t *node, void *arg)
{
	nvlist_t *fmri = NULL;
	char *fmristr = NULL;
	int err;
	struct led_cb_arg *cbarg = (struct led_cb_arg *)arg;
	topo_faclist_t faclist, *lp;
	topo_led_state_t currmode;

	if (topo_node_resource(node, &fmri, &err) != 0 ||
	    topo_fmri_nvl2str(thp, fmri, &fmristr, &err) != 0) {
		(void) fprintf(stderr, "failed to get FMRI of node: %s",
		    topo_strerror(err));
		nvlist_free(fmri);
		return (TOPO_WALK_ERR);
	}
	nvlist_free(fmri);

	/*
	 * Check if this is the node matches the pattern we're looking for
	 * and bail, if not.
	 */
	if (fnmatch(cbarg->lcb_fmri, fmristr, 0) != 0) {
		topo_hdl_strfree(thp, fmristr);
		return (TOPO_WALK_NEXT);
	}
	(void) printf("Found node: %s\n", fmristr);
	topo_hdl_strfree(thp, fmristr);

	/*
	 * Ok, we found our node.  Now check if it has an indicator of the
	 * correct type associated with it.  If not, bail out.
	 */
	if (topo_node_facility(thp, node, TOPO_FAC_TYPE_INDICATOR,
	    cbarg->lcb_ledtype, &faclist, &err) != 0) {
		return (TOPO_WALK_NEXT);
	}

	for (lp = topo_list_next(&faclist.tf_list); lp != NULL;
	    lp = topo_list_next(lp)) {
		if (cbarg->lcb_ledmode == -1) {
			/* get op */
			if (topo_prop_get_uint32(lp->tf_node,
			    TOPO_PGROUP_FACILITY, TOPO_LED_MODE, &currmode,
			    &err) != 0) {
				(void) fprintf(stderr, "failed to get LED "
				    "mode: %s\n", topo_strerror(err));
			}
			(void) printf("%s LED mode is %s\n",
			    cbarg->lcb_ledtypestr,
			    currmode ? "ON" : "OFF");
		} else {
			/* set op */
			if (topo_prop_set_uint32(lp->tf_node,
			    TOPO_PGROUP_FACILITY, TOPO_LED_MODE,
			    TOPO_PROP_MUTABLE, cbarg->lcb_ledmode,
			    &err) != 0) {
				(void) fprintf(stderr, "failed to set LED "
				    "mode: %s\n", topo_strerror(err));

			}
			(void) printf("%s LED mode set to %s\n",
			    cbarg->lcb_ledtypestr,
			    cbarg->lcb_ledmode ? "ON" : "OFF");
		}
	}
}

int
main(int argc, char *argv[])
{
	topo_hdl_t *thp;
	topo_walk_t *twp;
	struct led_cb_arg cbarg = { 0 };
	char c, *root = "/", *mode = NULL;
	int err, status = 1;

	pname = argv[0];

	while (optind < argc) {
		while ((c = getopt(argc, argv, optstr)) != -1) {
			switch (c) {
			case 'm':
				mode = optarg;
				break;
			case 'R':
				root = optarg;
				break;
			case 't':
				cbarg.lcb_ledtypestr = optarg;
				break;
			default:
				usage();
				return (2);
			}
		}
		if (optind < argc) {
			if (cbarg.lcb_fmri != NULL) {
				usage();
				return (2);
			} else {
				cbarg.lcb_fmri = argv[optind++];
			}
		}
	}
	if (cbarg.lcb_fmri == NULL || mode == NULL ||
	    cbarg.lcb_ledtypestr == NULL) {
		(void) fprintf(stderr, "-f/-m/-t options are required\n");
		usage();
		return (2);
	}

	if (strcmp(cbarg.lcb_ledtypestr, "locate") == 0) {
		cbarg.lcb_ledtype = TOPO_LED_TYPE_LOCATE;
	} else if (strcmp(cbarg.lcb_ledtypestr, "service") == 0) {
		cbarg.lcb_ledtype = TOPO_LED_TYPE_SERVICE;
	} else if (strcmp(cbarg.lcb_ledtypestr, "ok2rm") == 0) {
		cbarg.lcb_ledtype = TOPO_LED_TYPE_OK2RM;
	} else {
		(void) fprintf(stderr, "invalid LED type: %s\n",
		    cbarg.lcb_ledtypestr);
		usage();
		return (2);
	}

	if (strcmp(mode, "get") == 0) {
		cbarg.lcb_ledmode = -1;
	} else if (strcmp(mode, "on") == 0) {
		cbarg.lcb_ledmode = TOPO_LED_STATE_ON;
	} else if (strcmp(mode, "off") == 0) {
		cbarg.lcb_ledmode = TOPO_LED_STATE_OFF;
	} else {
		(void) fprintf(stderr, "invalid mode: %s\n", mode);
		usage();
		return (2);
	}

	if ((thp = topo_open(TOPO_VERSION, root, &err)) == NULL) {
		(void) fprintf(stderr, "failed to get topo handle: %s\n",
		    topo_strerror(err));
		goto out;
	}
	if (topo_snap_hold(thp, NULL, &err) == NULL) {
		(void) fprintf(stderr, "failed to take topo snapshot: %s\n",
		    topo_strerror(err));
		goto out;
	}
	if ((twp = topo_walk_init(thp, "hc", ledcb, &cbarg, &err)) == NULL) {
		(void) fprintf(stderr, "failed to init topo walker: %s\n",
		    topo_strerror(err));
		goto out;
	}
	if (topo_walk_step(twp, TOPO_WALK_CHILD) == TOPO_WALK_ERR) {
		(void) fprintf(stderr, "failed to walk topology\n");
		topo_walk_fini(twp);
		goto out;
	}
	topo_walk_fini(twp);

	status = 0;
out:
	if (thp != NULL)  {
		topo_snap_release(thp);
		topo_close(thp);
	}
	return (status);
}
