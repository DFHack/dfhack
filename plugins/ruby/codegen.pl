#!/usr/bin/perl

use strict;
use warnings;

use XML::LibXML;

our @lines_rb;
our @lines_cpp;

sub indent_rb(&) {
    my ($sub) = @_;
    my @lines;
    {
        local @lines_rb;
        $sub->();
        @lines = map { "    " . $_ } @lines_rb;
    }
    push @lines_rb, @lines
}

sub rb_ucase {
    my ($name) = @_;
    return join("", map { ucfirst $_ } (split('_', $name)));
}

my %render_struct_field = (
    'global' => \&render_field_global,
    'number' => \&render_field_number,
    'container' => \&render_field_container,
);

my %render_global_type = (
    'enum-type' => \&render_global_enum,
    'struct-type' => \&render_global_class,
    'class-type' => \&render_global_class,
    'bitfield-type' => \&render_global_bitfield,
);

sub render_global_enum {
    my ($name, $type) = @_;
    my $value = -1;

    my $rbname = rb_ucase($name);
    push @lines_rb, "class $rbname";
    indent_rb {
        for my $item ($type->findnodes('child::enum-item')) {
            $value = $item->getAttribute('value') || ($value+1);
            my $elemname = $item->getAttribute('name'); # || "unk_$value";

            if ($elemname) {
                my $rbelemname = rb_ucase($elemname);
                push @lines_rb, "$rbelemname = $value";
            }
        }
    };
    push @lines_rb, "end";
}

sub render_global_bitfield {
    my ($name, $type) = @_;

    my $rbname = rb_ucase($name);
    push @lines_rb, "class $rbname < MemStruct";
    indent_rb {
        my $shift = 0;
        for my $field ($type->findnodes('child::ld:field')) {
            my $count = $field->getAttribute('count') || 1;
            print "bitfield $name !number\n" if (!($field->getAttribute('ld:meta') eq 'number'));
            my $fname = $field->getAttribute('name');
            push @lines_rb, "bits :$fname, $shift, $count" if ($fname);
            $shift += $count;
        }
    };
    push @lines_rb, "end";
}

sub render_global_class {
    my ($name, $type) = @_;

    #my $cppvar = 'v';  # for offsetof

    my $rbname = rb_ucase($name);
    my $parent = rb_ucase($type->getAttribute('inherits-from') || 'MemStruct');
    push @lines_rb, "class $rbname < $parent";
    indent_rb {
        render_struct_fields($type);
    };
    push @lines_rb, "end";
}

sub render_field_global {
    my ($name, $field) = @_;

    my $typename = $field->getAttribute('type-name');
    my $rbname = rb_ucase($typename);
    my $offset = "'offsetof_$name'";

    push @lines_rb, "global :$name, :$rbname, $offset";
}

sub render_field_number {
    my ($name, $field) = @_;

    my $subtype = $field->getAttribute('ld:subtype');
    my $offset = "'offsetof_$name'";

    push @lines_rb, "number :$name, :$subtype, $offset"
}

sub render_field_container {
    my ($name, $field) = @_;

    my $subtype = $field->getAttribute('ld:subtype');
    my $offset = "'offsetof_$name'";

    if ($subtype eq 'stl-vector') {
        my $elem = $field->findnodes('child::ld:item')->[0] or return;

        push @lines_rb, "vector :$name, :$subtype, $offset"
    } else {
        print "no render field container $subtype\n";
    }
}

sub render_struct_fields {
    my ($type) = @_;

    for my $field ($type->findnodes('child::ld:field')) {
        my $name = $field->getAttribute('name');
        #$name = $field->getAttribute('ld:anon-name') if (!$name);
        next if (!$name);
        my $meta = $field->getAttribute('ld:meta');

        my $renderer = $render_struct_field{$meta};
        if ($renderer) {
            $renderer->($name, $field);
        } else {
            print "no render field $meta\n";
        }
    }
}



my $input = $ARGV[0] || '../../library/include/df/codegen.out.xml';
my $output_rb = $ARGV[1] || 'ruby-autogen.rb';
my $output_cpp = $ARGV[2] || 'ruby-autogen.cpp';

my $doc = XML::LibXML->new()->parse_file($input);
my %global_types;
$global_types{$_->getAttribute('type-name')} = $_ foreach $doc->findnodes('/ld:data-definition/ld:global-type');

for my $name (sort { $a cmp $b } keys %global_types) {
    my $type = $global_types{$name};
    my $meta = $type->getAttribute('ld:meta');
    my $renderer = $render_global_type{$meta};
    if ($renderer) {
        $renderer->($name, $type);
    } else {
        print "no render global type $meta\n";
    }
}

for my $obj ($doc->findnodes('/ld:data-definition/ld:global-object')) {
    my $name = $obj->getAttribute('name');
}

open FH, ">$output_rb";
print FH "$_\n" for @lines_rb;
close FH;

open FH, ">$output_cpp";
print FH "$_\n" for @lines_cpp;
close FH;
