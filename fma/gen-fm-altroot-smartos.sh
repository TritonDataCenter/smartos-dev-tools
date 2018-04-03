#!/usr/bin/bash

#
# Copyright (c) 2018, Joyent, Inc.
#

readonly proto_files=(
    "etc/logadm.d/fmd.logadm.conf"
    "etc/net-snmp/snmp/mibs/SUN-FM-MIB.mib"
    "etc/net-snmp/snmp/mibs/SUN-IREPORT-MIB.mib"
    "lib/fm"
    "lib/svc/manifest/system/fm/notify-params.xml"
    "lib/svc/manifest/system/fmd.xml"
    "usr/include/fm/"
    "usr/lib/libipmi.so.1"
    "usr/lib/64/libipmi.so.1"
    "usr/lib/fm"
    "usr/lib/locale/C/LC_MESSAGES/AMD.mo"
    "usr/lib/locale/C/LC_MESSAGES/DISK.mo"
    "usr/lib/locale/C/LC_MESSAGES/FMD.mo"
    "usr/lib/locale/C/LC_MESSAGES/FMNOTIFY.mo"
    "usr/lib/locale/C/LC_MESSAGES/GMCA.mo"
    "usr/lib/locale/C/LC_MESSAGES/INTEL.mo"
    "usr/lib/locale/C/LC_MESSAGES/NXGE.mo"
    "usr/lib/locale/C/LC_MESSAGES/PCI.mo"
    "usr/lib/locale/C/LC_MESSAGES/PCIEX.mo"
    "usr/lib/locale/C/LC_MESSAGES/SENSOR.mo"
    "usr/lib/locale/C/LC_MESSAGES/SMF.mo"
    "usr/lib/locale/C/LC_MESSAGES/STORAGE.mo"
    "usr/lib/locale/C/LC_MESSAGES/SUNOS.mo"
    "usr/lib/locale/C/LC_MESSAGES/ZFS.mo"
    "usr/lib/mdb/proc"
    "usr/platform/i86pc/lib/fm/"
    "usr/sbin/fmadm"
    "usr/sbin/fmdump"
    "usr/sbin/fmstat"
    "usr/share/lib/xml/dtd/topology.dtd.1"
    "usr/share/man/man1m/fmadm.1m"
    "usr/share/man/man1m/fmd.1m"
    "usr/share/man/man1m/fmdump.1m"
    "usr/share/man/man1m/fmstat.1m"
)

function cleanup
{
	[[ -z $tmpfile ]] || rm -f $tmpfile
}

function fail
{
	echo "FAIL: $1"
	exit 1
}

function usage
{
	printf "Usage: gen-fma-altroot-smartos <smartos repo>\n\n"
	exit 2
}

trap cleanup EXIT

[[ -z $1 ]] && usage

readonly ws=$1
readonly tarball=/var/tmp/fma-altroot.tar

#
# Sanity check that what the user passed in looks like a valid smartos repo
# with a proto.
#
[[ -d $ws/.git ]] || fail "$ws doesn't look like a git repo"
[[ -d $ws/proto ]] || fail "$ws doesn't contain a valid proto area"

echo "Generating archive ... \c"
tmpfile=$(mktemp)
for path in "${proto_files[@]}"; do
	[[ -e $ws/proto/$path ]] || fail "path $path not found in proto area"
	echo $path >> $tmpfile
done

rm -f $tarball
(cd $ws/proto; gtar cvfp $tarball -T $tmpfile)
[[ $? == 0 ]] || fail "failed to create archive"
echo "done"

echo "Archive successfully created at $tarball"
exit 0
