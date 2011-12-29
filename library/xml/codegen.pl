#!/usr/bin/perl

use strict;
use warnings;

BEGIN {
    our $script_root = '.';
    if ($0 =~ /^(.*)[\\\/][^\\\/]*$/) {
        $script_root = $1;
        unshift @INC, $1;
    }
};

use XML::LibXML;
use XML::LibXSLT;

use Common;

use Enum;
use Bitfield;
use StructType;

my $input_dir = $ARGV[0] || '.';
my $output_dir = $ARGV[1] || 'codegen';

$main_namespace = $ARGV[2] || 'df';
$export_prefix = 'DFHACK_EXPORT ';

# Collect all type definitions from XML files

our $script_root;
my $parser = XML::LibXML->new();
my $xslt = XML::LibXSLT->new();
my @transforms =
    map { $xslt->parse_stylesheet_file("$script_root/$_"); }
    ('lower-1.xslt', 'lower-2.xslt');
my @documents;

for my $fn (sort { $a cmp $b } glob "$input_dir/*.xml") {
    local $filename = $fn;
    my $doc = $parser->parse_file($filename);
    $doc = $_->transform($doc) for @transforms;
    
    push @documents, $doc;
    add_type_to_hash $_ foreach $doc->findnodes('/ld:data-definition/ld:global-type');
}

# Generate text representations

my %type_handlers = (
    'enum-type' => \&render_enum_type,
    'bitfield-type' => \&render_bitfield_type,
    'class-type' => \&render_struct_type,
    'struct-type' => \&render_struct_type,
);

my %type_data;

for my $name (sort { $a cmp $b } keys %types) {
    local $typename = $name;
    local $filename = $type_files{$typename};
    local %weak_refs;
    local %strong_refs;

    eval {
        my $type = $types{$typename};
        my $meta = $type->getAttribute('ld:meta') or die "Null meta";

        # Emit the actual type definition
        my @code = with_emit {
            with_anon {
                my $handler = $type_handlers{$meta} or die "Unknown type meta: $meta\n";
                $handler->($type);
            };
        } 2;

        delete $weak_refs{$name};
        delete $strong_refs{$name};
        
        # Add wrapping
        my @all = with_emit {
            my $def = type_header_def($typename);
            emit "#ifndef $def";
            emit "#define $def";

            for my $strong (sort { $a cmp $b } keys %strong_refs) {
                my $sdef = type_header_def($strong);
                emit "#ifndef $sdef";
                emit "#include \"$strong.h\"";
                emit "#endif";            
            }

            emit_block {
                for my $weak (sort { $a cmp $b } keys %weak_refs) {
                    next if $strong_refs{$weak};
                    my $ttype = $types{$weak};
                    my $tstr = 'struct';
                    $tstr = 'enum' if $ttype->nodeName eq 'enum-type';
                    $tstr = 'union' if $ttype->nodeName eq 'bitfield-type';
                    $tstr = 'union' if ($ttype->nodeName eq 'struct-type' && is_attr_true($ttype,'is-union'));
                    emit $tstr, ' ', $weak, ';';
                }

                push @lines, @code;
            } "namespace $main_namespace ";

            emit "#endif";
        };
        
        $type_data{$typename} = \@all;
    };
    if ($@) {
        print 'Error: '.$@."Type $typename in $filename ignored\n";
    }
}

# Write output files

mkdir $output_dir;

{
    # Delete the old files
    for my $name (glob "$output_dir/*.h") {
        unlink $name;
    }
    for my $name (glob "$output_dir/*.inc") {
        unlink $name;
    }
    unlink "$output_dir/codegen.out.xml";

    # Write out the headers
    local $, = "\n";
    local $\ = "\n";

    for my $name (keys %type_data) {
        open FH, ">$output_dir/$name.h";
        print FH "/* THIS FILE WAS GENERATED. DO NOT EDIT. */";
        print FH @{$type_data{$name}};
        close FH;
    }

    # Write out the static file
    for my $tag (keys %static_lines) {
        my $name = $output_dir.'/static'.($tag?'.'.$tag:'').'.inc';
        open FH, ">$name";
        print FH "/* THIS FILE WAS GENERATED. DO NOT EDIT. */";
        for my $name (sort { $a cmp $b } keys %{$static_includes{$tag}}) {
            print FH "#include \"$name.h\"";
        }
        print FH "namespace $main_namespace {";
        print FH @{$static_lines{$tag}};
        print FH '}';
        close FH;
    }

    # Write an xml file with all types
    open FH, ">$output_dir/codegen.out.xml";
    print FH '<ld:data-definition xmlns:ld="http://github.com/peterix/dfhack/lowered-data-definition">';
    for my $doc (@documents) {
        for my $node ($doc->documentElement()->findnodes('*')) {
            print FH '    '.$node->toString();
        }
    }
    print FH '</ld:data-definition>';
    close FH;
}
