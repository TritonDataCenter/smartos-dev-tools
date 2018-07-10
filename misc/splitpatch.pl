#!/usr/bin/perl -w

#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright (c) 2018, Joyent, Inc.
#

sub usage
{
	print STDERR "\nusage: $prog <patch_file> <output_dir>\n";
	exit (2);
}

sub dump_patch($\@)
{
	my $outdir = $_[0];
	my @patch = @{$_[1]};
	
	`mkdir -p $outdir`;
	if ($? != 0) {
		print STDERR "Failed to create directory: $outdir\n";
		return (-1);
	}
	my $diffline = $patch[0];
	$diffline =~ /(\S+)$/;
	my $patch_filenm = $1;
	$patch_filenm =~ s/\//_/g;
	$patch_filenm = "$outdir/$patch_filenm";

	print "Creating patch $patch_filenm ...";
	if (! open(OUTPATCH, ">", "$patch_filenm")) {
		print STDERR "Failed to open $patch_filenm for writing\n";
		return (-1);
	}
	foreach (@patch) {
		print OUTPATCH $_;
	}
	close (OUTPATCH);

	print "done\n";

	return (0);
}

#
# Configure stderr/stdout to autoflush to ensure things get printed in the
# intended order.
#
select(STDERR);
$| = 1;
select(STDOUT);
$| = 1;

$prog = $0;
my $patchfile = $ARGV[0];
my $outdir = $ARGV[1];
my $status = 1;

if ($#ARGV != 1) {
	&usage;
}

open(PATCH, $patchfile) ||
    die "Failed to open patch file: $patchfile\n";

#
# Nothing fancy - just find the beginning of the first diff, and start
# pushing the contents into an array.  From then on, when we find the start
# of the next diff, we dump the contents of the array to a patch file,
# clear the array and rinse, repeat.
#
$patch_start = 0;
while (<PATCH>) {
	my $line = $_;
	if (/^diff/ && $patch_start == 0) {
		$patch_start = 1;
		push(@currpatch, $line);
	} elsif (/^diff/ && $patch_start == 1) {
		if (&dump_patch($outdir, \@currpatch) != 0) {
			close (PATCH);
			exit (1);
		}
		@currpatch = ();
		push(@currpatch, $line);
	} elsif ($patch_start == 1) {
		push(@currpatch, $line);
	}
}
close (PATCH);

#
# Pop the 2-line end marker off and write out the last patch
#
pop(@currpatch);
pop(@currpatch);
exit 1 if (&dump_patch($outdir, \@currpatch) != 0);

$status = 0;

exit ($status);
