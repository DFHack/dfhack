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
    return $name if ($name eq uc($name));
    return join("", map { ucfirst $_ } (split('_', $name)));
}

my %render_global_type = (
    'enum-type' => \&render_global_enum,
    'struct-type' => \&render_global_class,
    'class-type' => \&render_global_class,
    'bitfield-type' => \&render_global_bitfield,
);

my %render_struct_field = (
    'global' => \&render_field_global,
    'number' => \&render_field_number,
    'container' => \&render_field_container,
    'compound' => \&render_field_compound,
    'pointer' => \&render_field_pointer,
    'static-array' => \&render_field_array,
    'primitive' => \&render_field_primitive,
    'bytes' => \&render_field_bytes,
);

my %render_item = (
    'number' => \&render_item_number,
    'global' => \&render_item_global,
    'primitive' => \&render_item_primitive,
    'pointer' => \&render_item_pointer,
    'static-array' => \&render_item_array,
    'container' => \&render_item_container,
    'compound' => \&render_item_compound,
);

sub render_global_enum {
    my ($name, $type) = @_;

    my $rbname = rb_ucase($name);
    push @lines_rb, "class $rbname";
    indent_rb {
        render_enum_fields($type);
    };
    push @lines_rb, "end";
}

sub render_enum_fields {
    my ($type) = @_;

    my $value = -1;
    for my $item ($type->findnodes('child::enum-item')) {
        $value = $item->getAttribute('value') || ($value+1);
        my $elemname = $item->getAttribute('name'); # || "unk_$value";

        if ($elemname) {
            my $rbelemname = rb_ucase($elemname);
            push @lines_rb, "$rbelemname = $value";
        }
    }
}

sub render_global_bitfield {
    my ($name, $type) = @_;

    my $rbname = rb_ucase($name);
    push @lines_rb, "class $rbname < MemStruct";
    indent_rb {
        render_bitfield_fields($type);
    };
    push @lines_rb, "end";
}

sub render_bitfield_fields {
    my ($type) = @_;

    my $shift = 0;
    for my $field ($type->findnodes('child::ld:field')) {
        my $count = $field->getAttribute('count') || 1;
        my $name = $field->getAttribute('name');
        $name = $field->getAttribute('ld:anon-name') if (!$name);
        print "bitfield $name !number\n" if (!($field->getAttribute('ld:meta') eq 'number'));
        if ($count == 1) {
            push @lines_rb, "bit :$name, $shift" if ($name);
        } else {
            push @lines_rb, "bits :$name, $shift, $count" if ($name);
        }
        $shift += $count;
    }
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

    push @lines_rb, "global :$name, $offset, :$rbname";
}

sub render_field_number {
    my ($name, $field) = @_;

    my $subtype = $field->getAttribute('ld:subtype');
    my $offset = "'offsetof_$name'";
   $subtype = 'float' if ($subtype eq 's-float');

    push @lines_rb, "$subtype :$name, $offset"
}

sub render_field_compound {
    my ($name, $field) = @_;

    my $offset = "'offsetof_$name'";
    my $subtype = $field->getAttribute('ld:subtype');
    if (!$subtype || $subtype eq 'bitfield') {
        push @lines_rb, "compound(:$name, $offset) {";
        indent_rb {
            if (!$subtype) {
                render_struct_fields($field);
            } else {
                render_bitfield_fields($field);
            }
        };
        push @lines_rb, "}"
    } elsif ($subtype eq 'enum') {
        render_enum_fields($field);
    } else {
        print "no render compound $subtype\n";
    }
}

sub render_field_container {
    my ($name, $field) = @_;

    my $subtype = $field->getAttribute('ld:subtype');
    my $offset = "'offsetof_$name'";

    if ($subtype eq 'stl-deque') {
        push @lines_rb, "stl_deque :$name, $offset";
    } elsif ($subtype eq 'stl-bit-vector') {
        push @lines_rb, "stl_bitvector :$name, $offset";
    } elsif ($subtype eq 'stl-vector') {
        my $item = $field->findnodes('child::ld:item')->[0] or return;
        my $itemdesc = render_item($item);
        push @lines_rb, "stl_vector(:$name, $offset, $itemdesc";
    } elsif ($subtype eq 'df-array') {
        my $item = $field->findnodes('child::ld:item')->[0] or return;
        my $itemdesc = render_item($item);
        push @lines_rb, "df_array(:$name, $offset, $itemdesc";
    } elsif ($subtype eq 'df-linked-list') {
        my $item = $field->findnodes('child::ld:item')->[0] or return;
        my $itemdesc = render_item($item);
        push @lines_rb, "df_linklist(:$name, $offset, $itemdesc";
    } elsif ($subtype eq 'df-flagarray') {
        push @lines_rb, "df_flagarray :$name, $offset";
    } else {
        print "no render field container $subtype\n";
    }
}


sub render_field_pointer {
    my ($name, $field) = @_;

    my $offset = "'offsetof_$name'";
    my $item = $field->findnodes('child::ld:item')->[0] or return;
    my $itemdesc = render_item($item);
    push @lines_rb, "pointer(:$name, $offset, $itemdesc";
}

sub render_field_array {
    my ($name, $field) = @_;

    my $offset = "'offsetof_$name'";
    my $count = $field->getAttribute('count');
    my $item = $field->findnodes('child::ld:item')->[0] or return;
    my $itemdesc = render_item($item);
    push @lines_rb, "staticarray(:$name, $offset, $count, $itemdesc";
}

sub render_field_primitive {
    my ($name, $field) = @_;

    my $offset = "'offsetof_$name'";
    my $subtype = $field->getAttribute('ld:subtype');
    if ($subtype eq 'stl-string') {
        push @lines_rb, "string :$name, $offset";
    } else {
        print "no render primitive $subtype $name\n";
    }
}

sub render_field_bytes {
    my ($name, $field) = @_;

    my $offset = "'offsetof_$name'";
    my $subtype = $field->getAttribute('ld:subtype');
    if ($subtype eq 'padding') {
    } elsif ($subtype eq 'static-string') {
        my $size = $field->getAttribute('ld:subtype');
        push @lines_rb, "static_string :$name, $offset, $size";
    } else {
        print "no render field bytes $subtype $name\n";
    }
}

sub render_struct_fields {
    my ($type) = @_;

    for my $field ($type->findnodes('child::ld:field')) {
        my $name = $field->getAttribute('name');
        $name = $field->getAttribute('ld:anon-name') if (!$name);
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

sub render_item_number {
    my ($item) = @_;

    my $sz = $item->getAttribute('ld:bits') / 8;
    my $subtype = $item->getAttribute('ld:subtype');
    $subtype = 'float' if ($subtype eq 's-float');
    return "$sz) { a_$subtype }";
}

sub render_item_global {
    my ($item) = @_;

    my $typename = $item->getAttribute('type-name');
    my $rbname = rb_ucase($typename);
    return "nil) { a_global :$rbname }";
}

sub render_item_primitive {
    my ($item) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    if ($subtype eq 'stl-string') {
        return "nil) { a_string }";
    } else {
        print "no render a_primitive $subtype\n";
        return "nil)";
    }
}

sub render_item_pointer {
    my ($item) = @_;

    my $pitem = $item->findnodes('child::ld:item')->[0];
    my $desc;
    if ($pitem) {
        $desc = render_item($pitem);
    } else {
        $desc = 'nil)';
    }
    return "4) { a_pointer($desc }";
}

sub render_item_array {
    my ($item) = @_;

    print "no render item array\n";
    return "nil)";
}

sub render_item_container {
    my ($item) = @_;

    print "no render item container\n";
    return "nil)";
}

sub render_item_compound {
    my ($item) = @_;

    print "no render item compound\n";
    return "nil)";
}

sub render_item {
    my ($item) = @_;
    my $meta = $item->getAttribute('ld:meta');

    my $renderer = $render_item{$meta};
    if ($renderer) {
        return $renderer->($item);
    } else {
        print "no render field $meta\n";
        return "nil)";
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
