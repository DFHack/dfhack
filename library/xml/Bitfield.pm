package Bitfield;

use utf8;
use strict;
use warnings;

BEGIN {
    use Exporter  ();
    our $VERSION = 1.00;
    our @ISA     = qw(Exporter);
    our @EXPORT  = qw( &render_bitfield_core &render_bitfield_type );
    our %EXPORT_TAGS = ( ); # eg: TAG => [ qw!name1 name2! ],
    our @EXPORT_OK   = qw( );
}

END { }

use XML::LibXML;

use Common;

sub render_bitfield_core {
    my ($name, $tag) = @_;

    emit_block {
        emit get_primitive_base($tag), ' whole;';

        emit_block {
            for my $item ($tag->findnodes('child::ld:field')) {
                ($item->getAttribute('ld:meta') eq 'number' &&
                    $item->getAttribute('ld:subtype') eq 'flag-bit')
                    or die "Invalid bitfield member: ".$item->toString."\n";

                check_bad_attrs($item);
                my $name = ensure_name $item->getAttribute('name');
                my $size = $item->getAttribute('count') || 1;
                emit "unsigned ", $name, " : ", $size, ";";
            }
        } "struct ", " bits;";

        emit $name, '() : whole(0) {};';
    } "union $name ", ";";
}

sub render_bitfield_type {
    my ($tag) = @_;
    render_bitfield_core($typename,$tag);
}

1;
