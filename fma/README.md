# fma
This is a collection of scripts and utilties that I use while developing
changes in the FMA and libtopo areas.

dump-fmd-ckpt
-------------
The fmd management daemon (fmd) checkpoints the state of plugin modules in
a custom binary format called FCF.  The format is described in:
usr/src/cmd/fm/fmd/common/fmd_ckpt.c

These files are persisted in /var/fm/fmd/ckpt/<module>/

This CLI decodes/dumps an FCF file.

fmdev
-----
This is utility for testing/exercising the ioctl's implemented by the "fm" pseudo-driver.
Currently it supports calling the following ioctls:
- FM_IOC_VERSIONS
- FM_IOC_PHYSCPU_INFO
- FM_IOC_GENTOPO_LEGACY


ufmdev
------
This is a utility for testing/exercising the ioctl's implemented by the "ufm" pseudo-driver.
Currently it supports calling the following ioctls:
- UFM_IOC_GETCAPS
- UFM_IOC_REPORTSZ
- UFM_IOC_REPORT

gen-diagcode
------------
This utility takes an FM dictionary name and an event class and computes the
diagcode.  This is useful when you're adding a new diagnosis.

gen-fma-altroot.sh
------------------
libtopo and libipmi changes generally need to be tested on bare metal.  Often
when developing such changes it is useful to install those changes on a very
specific platform and then run "fmtopo", "ipmitopo" or "fmsim".  In some cases
those platforms may be a production or staging system where it's not appropriate
to replace the installed bits with developer bits.  Fortunately, libtopo can be
configured to operate on an alternate root image.  This script creates a tarball
from an illumos proto area containing a very minimal subset of files needed to 
test changes to libtopo.  The tarball can be expanded into a temp directory on
the target system (e.g. /var/tmp/altroot) and then you can point tools like
fmtopo at it (i.e. fmtopo -R /var/tmp/altroot) to safely test the changes on
a platform without actually altering the installed bits.

gen-fma-altroot-smartos.sh
--------------------------
Similar idea to the previous script.  The difference here is that we can't base
the contents of the archive on what gets delivered by the service/fault-management
IPS package because the smartos build doesn't build a repo as part of building
illumos-joyent.  Also, some of the libtopo-related files come from the base
smartos-live repo.  So instead we resort to a hardcoded list of pathnames in
the script.

topo-indicator
--------------
This CLI can be used to get or set the state of any chassis indicator that
is exposed via libtopo.

fminject-files
--------------
This contains template of input files for the fminject utility, which
can be used to inject FM events.
