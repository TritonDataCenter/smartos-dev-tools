# IPMI utilities
These are a set of IPMI command-line utilties.  Much of this functionality
already exists in some form in ipmitool.  But the point of these CLIs is that
they are written to both leverage and test functionality in libipmi.

chassis-ident
-------------
This utility can be used to get or set the state of the chassis identity
indicator, if supported by the platform.

dump-sdr
--------
This utility iterates through the Sensor Data Repository (SDR) and dumps some
information about each record.

dump-sp-info
------------
This utility dumps the firmware version and network configuration of the
service processor (SP), if present.

read-sensor
----------
Simple utility that will read a sensor when given either an IPMI entity name or
a combination of entity name with entity ID and entity instance.  I wrote this
primarily to test ipmi_fru_lookup_precise().
