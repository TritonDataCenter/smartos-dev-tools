#!/usr/bin/bash

#
# Copyright (c) 2018, Joyent, Inc.
#

readonly arch=$(uname -p)

function fail
{
	echo "FAIL: $1"
	exit 1
}

function usage
{
	printf "Usage: gen-fma-altroot <built illumos repo>\n\n"
	exit 2
}

[[ -z $1 ]] && usage

readonly ws=$1

#
# Sanity check that what the user passed in looks like a valid illumos repo
# with a proto area and built packages.
#
[[ -d $ws/.git ]] || fail "$ws doesn't look like a git repo"

proto=$ws/proto/root_$arch
[[ -d $proto ]] || fail "$ws doesn't contain a valid proto area"

pkgrepo=$ws/packages/$arch/nightly/repo.redist
[[ -d $pkgrepo ]] || fail "repo doesn't contain a built package repo"

#
# Create a list of files in the fault-management pkg, pruning out directories.
# We then pass this list to gtar to build our archive.
#
pkgfmri="service/fault-management"
tmpdir=$(/usr/bin/mktemp -d -p /var/tmp)

echo "Generating list of files to archive ... \c"
touch $tmpdir/filelist.txt
for path in `/usr/bin/pkg contents -g $pkgrepo -H $pkgfmri`; do
	[[ -d $proto/$path ]] || echo $path >> $tmpdir/filelist.txt	
done
[[ $? == 0 ]] || fail "failed to get contents of $pkgfmri"
echo "done"

echo "Creating archive ... \c"
(cd $proto; gtar cfp $tmpdir/fma-altroot.tar -T $tmpdir/filelist.txt)
[[ $? == 0 ]] || fail "failed to create archive"
echo "done"

echo "Archive successfully created at $tmpdir/fma-altroot.tar"
exit 0
