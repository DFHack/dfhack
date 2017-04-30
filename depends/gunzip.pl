#!/usr/bin/perl

use strict;
use warnings;

use IO::Uncompress::Gunzip qw(gunzip $GunzipError);

my %args = map { $_ => 1 } @ARGV;

my $in_file = $ARGV[0] or die "no input file";
# remove extension
(my $out_file = $in_file) =~ s{\.[^.]+$}{};

if (! -f $in_file) {
    die "input file does not exist: \"$in_file\"\n";
}
if (-f $out_file and !exists($args{'--force'})) {
    die "output file exists, not overwriting: \"$out_file\"";
}

gunzip $in_file => $out_file, BinModeOut => 1 or die "gunzip failed: $GunzipError\n";
