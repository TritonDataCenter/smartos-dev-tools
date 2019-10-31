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
#include <libnvpair.h>
#include <string.h>
#include <stropts.h>
#include <unistd.h>
#include <sys/types.h>

#include "mptsas_ioctl.h"

#define	EXIT_USAGE	2

static const char *pname;
static const char optstr[] = "d:i:";

static void
usage()
{
	(void) fprintf(stderr, "usage: %s -d <devctl devpath> -i <ioctl>\n\n"
	    "where \'ioctl\' can be:\n"
	    "getadapterdata\ngetdiskinfo\ngetpciinfo\ngetconninfo\n\n", pname);
}

static int
do_getconninfo(int fd)
{
	mptsas_get_connector_info_t ioc = { 0 };
	nvlist_t *nvl = NULL;
	uint_t nphys;

	if (ioctl(fd, MPTIOCTL_GET_CONNECTOR_INFO, &ioc) < 0) {
		(void) fprintf(stderr, "MPTIOCTL_GET_CONNECTOR_INFO ioctl "
		    "failed (%s)\n", strerror(errno));
		return (-1);
	}
	(void) printf("Required buffer size = %u\n", ioc.mci_bufsz);

	if ((ioc.mci_buf = malloc(ioc.mci_bufsz)) == NULL)
		return (-1);

	if (ioctl(fd, MPTIOCTL_GET_CONNECTOR_INFO, &ioc) < 0) {
		(void) fprintf(stderr, "MPTIOCTL_GET_CONNECTOR_INFO ioctl "
		    "failed (%s)\n", strerror(errno));
		free(ioc.mci_buf);
		return (-1);
	}

	if (nvlist_unpack(ioc.mci_buf, ioc.mci_bufsz, &nvl,
	    NV_ENCODE_NATIVE) != 0) {
		(void) fprintf(stderr, "failed to unpack nvlist");
		free(ioc.mci_buf);
		return (-1);
	}
	free(ioc.mci_buf);

	nvlist_print(stdout, nvl);

	return (0);
}

static int
do_getdiskinfo(int fd)
{
	mptsas_get_disk_info_t ioc = { 0 };

	if (ioctl(fd, MPTIOCTL_GET_DISK_INFO, &ioc) < 0) {
		(void) fprintf(stderr, "MPTIOCTL_GET_DISK_INFO ioctl "
		    "failed (%s)\n", strerror(errno));
		return (-1);
	}
	(void) printf("Number of disks: %u\n", ioc.DiskCount);

	ioc.DiskInfoArraySize = ioc.DiskCount * sizeof (mptsas_disk_info_t);
	if ((ioc.PtrDiskInfoArray = malloc(ioc.DiskInfoArraySize)) == NULL)
		return (-1);

	if (ioctl(fd, MPTIOCTL_GET_DISK_INFO, &ioc) < 0) {
		(void) fprintf(stderr, "MPTIOCTL_DISK_INFO ioctl "
		    "failed (%s)\n", strerror(errno));
		free(ioc.PtrDiskInfoArray);
		return (-1);
	}

	for (uint_t i = 0; i < ioc.DiskCount; i++) {
		(void) printf("SAS Address: %" PRIx64 "\n",
		    ioc.PtrDiskInfoArray[i].SasAddress);
		(void) printf("Instance: %u\n",
		    ioc.PtrDiskInfoArray[i].Instance);
		(void) printf("Enclosure: %u\n",
		    ioc.PtrDiskInfoArray[i].Enclosure);
		(void) printf("Slot: %u\n\n", ioc.PtrDiskInfoArray[i].Slot);
	}

	free(ioc.PtrDiskInfoArray);

	return (0);
}

static int
do_getadapter(int fd)
{
	mptsas_adapter_data_t ioc = { 0 };
	char *type4 = "SAS-2", *type6 = "SAS-3", *typeX = "Unknown", *type;
	char driver_ver[33];

	if (ioctl(fd, MPTIOCTL_GET_ADAPTER_DATA, &ioc) < 0) {
		(void) fprintf(stderr, "MPTIOCTL_GET_ADAPTER_DATA ioctl "
		    "failed (%s)\n", strerror(errno));
		return (-1);
	}

	switch (ioc.AdapterType) {
	case 4:
		type = type4;
		break;
	case 6:
		type = type6;
		break;
	default:
		type = typeX;
		break;
	}
	(void) printf("Adapter Type: %u (%s)\n", ioc.AdapterType, type);
	(void) printf("PCI Function: %u\n", ioc.MpiPortNumber);
	(void) printf("PCI Device: %u\n", ioc.PCIDeviceHwId);
	(void) printf("Subsystem ID: 0x%x\n", ioc.SubSystemId);
	(void) printf("Subsystem Vendor ID: 0x%x\n", ioc.SubsystemVendorId);
	(void) printf("MPI Firmare Version: %u\n", ioc.MpiFirmwareVersion);
	(void) printf("BIOS Version: %u\n", ioc.BiosVersion);
	(void) strcpy(driver_ver, ioc.DriverVersion);
	driver_ver[32] = '\0';
	(void) printf("Driver Version: %s\n", driver_ver);
	(void) printf("SCSI ID: %u\n", ioc.ScsiId);

	return (0);
}

static int
do_getpciinfo(int fd)
{
	mptsas_pci_info_t ioc = { 0 };

	if (ioctl(fd, MPTIOCTL_GET_PCI_INFO, &ioc) < 0) {
		(void) fprintf(stderr, "MPTIOCTL_GET_ADAPTER_DATA ioctl "
		    "failed (%s)\n", strerror(errno));
		return (-1);
	}

	(void) printf("PCI Bus: %x\n", ioc.BusNumber);
	(void) printf("PCI Device: %x\n", ioc.DeviceNumber);
	(void) printf("PCI Function: %x\n", ioc.FunctionNumber);
	(void) printf("Interrupt Vector: %u\n", ioc.InterruptVector);

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
				return (EXIT_USAGE);
			}
		}
	}

	if (ioc == NULL || devpath == NULL) {
		(void) fprintf(stderr, "-d/-i options are required\n");
		usage();
		return (EXIT_USAGE);
	}

	if (strcasecmp(ioc, "getconninfo") == 0) {
		cmd = MPTIOCTL_GET_CONNECTOR_INFO;
	} else if (strcasecmp(ioc, "getadapterdata") == 0) {
		cmd = MPTIOCTL_GET_ADAPTER_DATA;
	} else if (strcasecmp(ioc, "getdiskinfo") == 0) {
		cmd = MPTIOCTL_GET_DISK_INFO;
	} else if (strcasecmp(ioc, "getpciinfo") == 0) {
		cmd = MPTIOCTL_GET_PCI_INFO;
	} else {
		(void) fprintf(stderr, "invalid ioctl name\n");
		usage();
		return (EXIT_USAGE);
	}

	if ((fd = open(devpath, O_RDONLY)) < 0) {
		(void) fprintf(stderr, "failed to open %s (%s)\n", devpath,
		    strerror(errno));
		return (EXIT_FAILURE);
	}

	switch (cmd) {
	case MPTIOCTL_GET_CONNECTOR_INFO:
		ret = do_getconninfo(fd);
		break;
	case MPTIOCTL_GET_ADAPTER_DATA:
		ret = do_getadapter(fd);
		break;
	case MPTIOCTL_GET_DISK_INFO:
		ret = do_getdiskinfo(fd);
		break;
	case MPTIOCTL_GET_PCI_INFO:
		ret = do_getpciinfo(fd);
		break;
	default:
		break;
	}

out:
	(void) close(fd);

	return ((ret < 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}
