#!/usr/bin/perl

# A replacement for "cmake -E copy_if_different" that supports multiple files,
# which old cmake versions do not support

# Usage: copy-if-different.pl src-file [src-file...] dest-dir

use strict;
use warnings;

use Digest::SHA;
use File::Basename;
use File::Copy;

sub sha_file {
    my $filename = shift;
    my $sha = Digest::SHA->new(256);
    $sha->addfile($filename);
    return $sha->hexdigest;
}

my $dest_dir = pop @ARGV or die "no destination dir";
-d $dest_dir or die "not a directory: $dest_dir";
my @src_files = @ARGV or die "no source files";

foreach my $file (@src_files) {
    my $dest = "$dest_dir/" . basename($file);
    next if -f $dest && sha_file($file) eq sha_file($dest);
    copy($file, $dest);
}
