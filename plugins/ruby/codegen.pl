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

my %global_type_renderer = (
    'enum-type' => \&render_global_enum,
    'struct-type' => \&render_global_class,
    'class-type' => \&render_global_class,
    'bitfield-type' => \&render_global_bitfield,
);

my %item_renderer = (
    'global' => \&render_item_global,
    'number' => \&render_item_number,
    'container' => \&render_item_container,
    'compound' => \&render_item_compound,
    'pointer' => \&render_item_pointer,
    'static-array' => \&render_item_staticarray,
    'primitive' => \&render_item_primitive,
    'bytes' => \&render_item_bytes,
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
sub render_struct_fields {
    my ($type) = @_;

    for my $field ($type->findnodes('child::ld:field')) {
        my $name = $field->getAttribute('name');
        $name = $field->getAttribute('ld:anon-name') if (!$name);
        next if (!$name);
        my $offset = "'TODOoffsetof_$name'";

        push @lines_rb, "field(:$name, $offset) {";
        indent_rb {
            render_item($field);
        };
        push @lines_rb, "}";
    }
}

sub render_item {
    my ($item) = @_;
    return if (!$item);

    my $meta = $item->getAttribute('ld:meta');

    my $renderer = $item_renderer{$meta};
    if ($renderer) {
        $renderer->($item);
    } else {
        print "no render item $meta\n";
    }
}

sub render_item_global {
    my ($item) = @_;

    my $typename = $item->getAttribute('type-name');
    my $rbname = rb_ucase($typename);

    push @lines_rb, "global :$rbname";
}

sub render_item_number {
    my ($item) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    $subtype = $item->getAttribute('base-type') if ($subtype eq 'enum');
    $subtype = 'int32_t' if (!$subtype);

         if ($subtype eq 'int64_t') {
        push @lines_rb, 'number 64, true';
    } elsif ($subtype eq 'uint32_t') {
        push @lines_rb, 'number 32, false';
    } elsif ($subtype eq 'int32_t') {
        push @lines_rb, 'number 32, true';
    } elsif ($subtype eq 'uint16_t') {
        push @lines_rb, 'number 16, false';
    } elsif ($subtype eq 'int16_t') {
        push @lines_rb, 'number 16, true';
    } elsif ($subtype eq 'uint8_t') {
        push @lines_rb, 'number 8, false';
    } elsif ($subtype eq 'int8_t') {
        push @lines_rb, 'number 8, false';
    } elsif ($subtype eq 'bool') {
        push @lines_rb, 'number 8, true';
    } elsif ($subtype eq 's-float') {
        push @lines_rb, 'float';
    } else {
        print "no render number $subtype\n";
    }
}

sub render_item_compound {
    my ($item) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    if (!$subtype || $subtype eq 'bitfield') {
        push @lines_rb, "compound {";
        indent_rb {
            if (!$subtype) {
                render_struct_fields($item);
            } else {
                render_bitfield_fields($item);
            }
        };
        push @lines_rb, "}"
    } elsif ($subtype eq 'enum') {
        # declare constants
        render_enum_fields($item);
        # actual field
        render_item_number($item);
    } else {
        print "no render compound $subtype\n";
    }
}

sub render_item_container {
    my ($item) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    my $rbmethod = join('_', split('-', $subtype));
    my $tg = $item->findnodes('child::ld:item')->[0];
    if ($tg) {
        my $tglen = get_tglen($tg);
        push @lines_rb, "$rbmethod($tglen) {";
        indent_rb {
            render_item($tg);
        };
        push @lines_rb, "}";
    } else {
        push @lines_rb, "$rbmethod";
    }
}

sub render_item_pointer {
    my ($item) = @_;

    my $tg = $item->findnodes('child::ld:item')->[0];
    my $tglen = get_tglen($tg);
    push @lines_rb, "pointer($tglen) {";
    indent_rb {
        render_item($tg);
    };
    push @lines_rb, "}";
}

sub render_item_staticarray {
    my ($item) = @_;

    my $count = $item->getAttribute('count');
    my $tg = $item->findnodes('child::ld:item')->[0];
    my $tglen = get_tglen($tg);
    push @lines_rb, "static_array($count, $tglen) {";
    indent_rb {
        render_item($tg);
    };
    push @lines_rb, "}";
}

sub render_item_primitive {
    my ($item) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    if ($subtype eq 'stl-string') {
        push @lines_rb, "stl_string";
    } else {
        print "no render primitive $subtype\n";
    }
}

sub render_item_bytes {
    my ($item) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    if ($subtype eq 'padding') {
    } elsif ($subtype eq 'static-string') {
        my $size = $item->getAttribute('size');
        push @lines_rb, "static_string($size)";
    } else {
        print "no render bytes $subtype\n";
    }
}


sub get_tglen {
    my ($tg) = @_;
    if (!$tg) { return 'nil'; }

    my $meta = $tg->getAttribute('ld:meta');
    if ($meta eq 'number') {
        return $tg->getAttribute('ld:bits')/8;
    } elsif ($meta eq 'pointer') {
        return 4;
    } else {
        # TODO
        return "'TODOsizeof($meta)'";
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
    my $renderer = $global_type_renderer{$meta};
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
