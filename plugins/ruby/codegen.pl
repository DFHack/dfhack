#!/usr/bin/perl

use strict;
use warnings;

use XML::LibXML;

our @lines_rb;

my $os = $ARGV[2] or die('os not provided (argv[2])');
if ($os =~ /linux/i or $os =~ /darwin/i) {
    $os = 'linux';
} elsif ($os =~ /windows/i) {
    $os = 'windows';
} else {
    die "Unknown OS: " . $ARGV[2] . "\n";
}

my $arch = $ARGV[3] or die('arch not provided (argv[3])');
if ($arch =~ /64/i) {
    $arch = 64;
} elsif ($arch =~ /32/i) {
    $arch = 32;
} else {
    die "Unknown architecture: " . $ARGV[3] . "\n";
}

# 32 bits on Windows and 32-bit *nix, 64 bits on 64-bit *nix
my $SIZEOF_LONG;
if ($os eq 'windows' || $arch == 32) {
    $SIZEOF_LONG = 4;
} else {
    $SIZEOF_LONG = 8;
}

my $SIZEOF_PTR = ($arch == 64) ? 8 : 4;

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


my %global_types;
our $current_typename;

sub render_global_enum {
    my ($name, $type) = @_;

    my $rbname = rb_ucase($name);
    push @lines_rb, "class $rbname < MemHack::Enum";
    indent_rb {
        render_enum_fields($type);
    };
    push @lines_rb, "end\n";
}

sub render_enum_fields {
    my ($type) = @_;

    push @lines_rb, "ENUM = Hash.new";
    push @lines_rb, "NUME = Hash.new";

    my %attr_type;
    my %attr_list;
    render_enum_initattrs($type, \%attr_type, \%attr_list);

    my $value = -1;
    for my $item ($type->findnodes('child::enum-item'))
    {
        $value = $item->getAttribute('value') || ($value+1);
        my $elemname = $item->getAttribute('name'); # || "unk_$value";

        if ($elemname)
        {
            my $rbelemname = rb_ucase($elemname);
            push @lines_rb, "ENUM[$value] = :$rbelemname ; NUME[:$rbelemname] = $value";
            for my $iattr ($item->findnodes('child::item-attr'))
            {
                my $attr = render_enum_attr($rbelemname, $iattr, \%attr_type, \%attr_list);
                $lines_rb[$#lines_rb] .= ' ; ' . $attr;
            }
        }
    }
}

sub render_enum_initattrs {
    my ($type, $attr_type, $attr_list) = @_;

    for my $attr ($type->findnodes('child::enum-attr'))
    {
        my $rbattr = rb_ucase($attr->getAttribute('name'));
        my $typeattr = $attr->getAttribute('type-name');
        # find how we need to encode the attribute values: string, symbol (for enums), raw (number, bool)
        if ($typeattr) {
            if ($global_types{$typeattr}) {
                $attr_type->{$rbattr} = 'symbol';
            } else {
                $attr_type->{$rbattr} = 'naked';
            }
        } else {
            $attr_type->{$rbattr} = 'quote';
        }

        my $def = $attr->getAttribute('default-value');
        if ($attr->getAttribute('is-list'))
        {
            push @lines_rb, "$rbattr = Hash.new { |h, k| h[k] = [] }";
            $attr_list->{$rbattr} = 1;
        }
        elsif ($def)
        {
            $def = ":$def" if ($attr_type->{$rbattr} eq 'symbol');
            $def =~ s/'/\\'/g if ($attr_type->{$rbattr} eq 'quote');
            $def = "'$def'" if ($attr_type->{$rbattr} eq 'quote');
            push @lines_rb, "$rbattr = Hash.new($def)";
        }
        else
        {
            push @lines_rb, "$rbattr = Hash.new";
        }
    }
}

sub render_enum_attr {
    my ($rbelemname, $iattr, $attr_type, $attr_list) = @_;

    my $ian = $iattr->getAttribute('name');
    my $iav = $iattr->getAttribute('value');
    my $rbattr = rb_ucase($ian);

    my $op = ($attr_list->{$rbattr} ? '<<' : '=');

    $iav = ":$iav" if ($attr_type->{$rbattr} eq 'symbol');
    $iav =~ s/'/\\'/g if ($attr_type->{$rbattr} eq 'quote');
    $iav = "'$iav'" if ($attr_type->{$rbattr} eq 'quote');

    return "${rbattr}[:$rbelemname] $op $iav";
}


sub render_global_bitfield {
    my ($name, $type) = @_;

    my $rbname = rb_ucase($name);
    push @lines_rb, "class $rbname < MemHack::Compound";
    indent_rb {
        render_bitfield_fields($type);
    };
    push @lines_rb, "end\n";
}

sub render_bitfield_fields {
    my ($type) = @_;

    push @lines_rb, "field(:_whole, 0) {";
    indent_rb {
        render_item_number($type);
    };
    push @lines_rb, "}";

    my $shift = 0;
    for my $field ($type->findnodes('child::ld:field'))
    {
        my $count = $field->getAttribute('count') || 1;
        my $name = $field->getAttribute('name');
        my $type = $field->getAttribute('type-name');
        my $enum = rb_ucase($type) if ($type and $global_types{$type});
        $name = $field->getAttribute('ld:anon-name') if (!$name);
        print "bitfield $name !number\n" if (!($field->getAttribute('ld:meta') eq 'number'));

        if ($name)
        {
            if ($enum) {
                push @lines_rb, "field(:$name, 0) { bits $shift, $count, $enum }";
            } elsif ($count == 1) {
                push @lines_rb, "field(:$name, 0) { bit $shift }";
            } else {
                push @lines_rb, "field(:$name, 0) { bits $shift, $count }";
            }
        }

        $shift += $count;
    }
}


my %seen_class;
our $compound_off;
our $compound_pointer;

sub render_global_class {
    my ($name, $type) = @_;

    my $meta = $type->getAttribute('ld:meta');
    my $rbname = rb_ucase($name);

    # ensure pre-definition of ancestors
    my $parent = $type->getAttribute('inherits-from');
    render_global_class($parent, $global_types{$parent}) if ($parent and !$seen_class{$parent});

    return if $seen_class{$name};
    $seen_class{$name}++;

    local $compound_off = 0;
    $compound_off = $SIZEOF_PTR if ($meta eq 'class-type');
    $compound_off = sizeof($global_types{$parent}) if $parent;
    local $current_typename = $rbname;

    my $rtti_name;
    if ($meta eq 'class-type')
    {
        $rtti_name = $type->getAttribute('original-name') ||
        $type->getAttribute('type-name') ||
        $name;
    }

    my $rbparent = ($parent ? rb_ucase($parent) : 'MemHack::Compound');
    push @lines_rb, "class $rbname < $rbparent";
    indent_rb {
        my $sz = sizeof($type);
        # see comment is sub sizeof ; but gcc has sizeof(cls) aligned
        $sz = align_field($sz, $SIZEOF_PTR) if $os eq 'linux' and $meta eq 'class-type';
        push @lines_rb, "sizeof $sz\n";

        push @lines_rb, "rtti_classname :$rtti_name\n" if $rtti_name;

        render_struct_fields($type);

        my $vms = $type->findnodes('child::virtual-methods')->[0];
        if ($vms)
        {
            my $voff = render_class_vmethods_voff($parent);
            render_class_vmethods($vms, $voff);
        }
    };
    push @lines_rb, "end\n";
}

sub render_struct_fields {
    my ($type) = @_;

    my $isunion = $type->getAttribute('is-union');

    for my $field ($type->findnodes('child::ld:field'))
    {
        my $name = $field->getAttribute('name');
        $name = $field->getAttribute('ld:anon-name') if (!$name);

        if (!$name and $field->getAttribute('ld:anon-compound'))
        {
            render_struct_fields($field);
        }
        else
        {
            $compound_off = align_field($compound_off, get_field_align($field));
            if ($name)
            {
                push @lines_rb, "field(:$name, $compound_off) {";
                indent_rb {
                    render_item($field);
                };
                push @lines_rb, "}";

                render_struct_field_refs($type, $field, $name);
            }
        }

        $compound_off += sizeof($field) if (!$isunion);
    }
}

# handle generating accessor for xml attributes ref-target, refers-to etc
sub render_struct_field_refs {
    my ($parent, $field, $name) = @_;

    my $reftg = $field->getAttribute('ref-target');

    my $refto = $field->getAttribute('refers-to');
    render_field_refto($parent, $name, $refto) if ($refto);

    my $meta = $field->getAttribute('ld:meta');
    my $item = $field->findnodes('child::ld:item')->[0];
    if ($meta and $meta eq 'container' and $item) {
        my $itemreftg = $item->getAttribute('ref-target');
        render_container_reftarget($parent, $item, $name, $itemreftg) if $itemreftg;
    } elsif ($reftg) {
        render_field_reftarget($parent, $field, $name, $reftg);
    }
}

sub render_field_reftarget {
    my ($parent, $field, $name, $reftg) = @_;

    my $tg = $global_types{$reftg};
    return if (!$tg);

    my $tgname = "${name}_tg";
    $tgname =~ s/_id(.?.?)_tg/_tg$1/;

    my $aux = $field->getAttribute('aux-value');
    if ($aux) {
        # minimal support (aims is unit.caste_tg)
        return if $aux !~ /^\$\$\.[^_][\w\.]+$/;
        $aux =~ s/\$\$\.//;

        for my $codehelper ($tg->findnodes('child::code-helper')) {
            if ($codehelper->getAttribute('name') eq 'find-instance') {
                my $helper = $codehelper->textContent;
                $helper =~ s/\$global/df/;
                $helper =~ s/\$\$/$aux/;
                $helper =~ s/\$/$name/;
                if ($helper =~ /^[\w\.\[\]]+$/) {
                    push @lines_rb, "def $tgname ; $helper ; end";
                }
            }
        }
        return;
    }

    my $tgvec = $tg->getAttribute('instance-vector');
    return if (!$tgvec);
    my $idx = $tg->getAttribute('key-field');

    $tgvec =~ s/^\$global/df/;
    return if $tgvec !~ /^[\w\.]+$/;

    for my $othername (map { $_->getAttribute('name') } $parent->findnodes('child::ld:field')) {
        $tgname .= '_' if ($othername and $tgname eq $othername);
    }

    if ($idx) {
        my $fidx = '';
        $fidx = ', :' . $idx if ($idx ne 'id');
        push @lines_rb, "def $tgname ; ${tgvec}.binsearch($name$fidx) ; end";
    } else {
        push @lines_rb, "def $tgname ; ${tgvec}[$name] ; end";
    }

}

sub render_field_refto {
    my ($parent, $name, $tgvec) = @_;

    $tgvec =~ s/^\$global/df/;
    $tgvec =~ s/\[\$\]$//;
    return if $tgvec !~ /^[\w\.]+$/;

    my $tgname = "${name}_tg";
    $tgname =~ s/_id(.?.?)_tg/_tg$1/;

    for my $othername (map { $_->getAttribute('name') } $parent->findnodes('child::ld:field')) {
        $tgname .= '_' if ($othername and $tgname eq $othername);
    }

    push @lines_rb, "def $tgname ; ${tgvec}[$name] ; end";
}

sub render_container_reftarget {
    my ($parent, $item, $name, $reftg) = @_;

    my $aux = $item->getAttribute('aux-value');
    return if ($aux); # TODO

    my $tg = $global_types{$reftg};
    return if (!$tg);
    my $tgvec = $tg->getAttribute('instance-vector');
    return if (!$tgvec);
    my $idx = $tg->getAttribute('key-field');

    $tgvec =~ s/^\$global/df/;
    return if $tgvec !~ /^[\w\.]+$/;

    my $tgname = "${name}_tg";
    $tgname =~ s/_id(.?.?)_tg/_tg$1/;

    for my $othername (map { $_->getAttribute('name') } $parent->findnodes('child::ld:field')) {
        $tgname .= '_' if ($othername and $tgname eq $othername);
    }

    if ($idx) {
        my $fidx = '';
        $fidx = ', :' . $idx if ($idx ne 'id');
        push @lines_rb, "def $tgname ; $name.map { |i| $tgvec.binsearch(i$fidx) } ; end";
    } else {
        push @lines_rb, "def $tgname ; $name.map { |i| ${tgvec}[i] } ; end";
    }
}

# return the size of the parent's vtables
sub render_class_vmethods_voff {
    my ($name) = @_;

    return 0 if !$name;

    my $type = $global_types{$name};
    my $parent = $type->getAttribute('inherits-from');

    my $voff = render_class_vmethods_voff($parent);
    my $vms = $type->findnodes('child::virtual-methods')->[0];

    for my $meth ($vms->findnodes('child::vmethod'))
    {
        $voff += $SIZEOF_PTR if $meth->getAttribute('is-destructor') and $os eq 'linux';
        $voff += $SIZEOF_PTR;
    }

    return $voff;
}

sub render_class_vmethods {
    my ($vms, $voff) = @_;

    for my $meth ($vms->findnodes('child::vmethod'))
    {
        my $name = $meth->getAttribute('name');

        if ($name)
        {
            my @argnames;
            my @argargs;

            # check if arguments need special treatment (eg auto-convert from symbol to enum value)
            for my $arg ($meth->findnodes('child::ld:field'))
            {
                my $nr = $#argnames + 1;
                my $argname = lcfirst($arg->getAttribute('name') || "arg$nr");
                push @argnames, $argname;

                if ($arg->getAttribute('ld:meta') eq 'global' and $arg->getAttribute('ld:subtype') eq 'enum') {
                    push @argargs, rb_ucase($arg->getAttribute('type-name')) . ".int($argname)";
                } else {
                    push @argargs, $argname;
                }
            }

            # write vmethod ruby wrapper
            push @lines_rb, "def $name(" . join(', ', @argnames) . ')';
            indent_rb {
                my $args = join('', map { ", $_" } @argargs);
                my $call = "DFHack.vmethod_call(self, $voff$args)";
                my $ret = $meth->findnodes('child::ret-type')->[0];
                render_class_vmethod_ret($call, $ret);
            };
            push @lines_rb, 'end';
        }

        # on linux, the destructor uses 2 entries
        $voff += $SIZEOF_PTR if $meth->getAttribute('is-destructor') and $os eq 'linux';
        $voff += $SIZEOF_PTR;
    }
}

sub render_class_vmethod_ret {
    my ($call, $ret) = @_;

    if (!$ret)
    {
        # method returns void, hide return value
        push @lines_rb, "$call ; nil";
        return;
    }

    my $retmeta = $ret->getAttribute('ld:meta') || '';
    if ($retmeta eq 'global')
    {
        # method returns an enum value: auto-convert to symbol
        my $retname = $ret->getAttribute('type-name');
        if ($retname and $global_types{$retname} and
            $global_types{$retname}->getAttribute('ld:meta') eq 'enum-type')
        {
            push @lines_rb, rb_ucase($retname) . ".sym($call)";
        }
        else
        {
            print "vmethod global nonenum $call\n";
            push @lines_rb, $call;
        }

    }
    elsif ($retmeta eq 'number')
    {
        # raw method call returns an int32, mask according to actual return type
        my $retsubtype = $ret->getAttribute('ld:subtype');
        my $retbits = sizeof($ret) * 8;
        push @lines_rb, "val = $call";
        if ($retsubtype eq 'bool')
        {
            push @lines_rb, "(val & 1) != 0";
        }
        elsif ($ret->getAttribute('ld:unsigned'))
        {
            push @lines_rb, "val & ((1 << $retbits) - 1)";
        }
        elsif ($retbits != 32)
        {
            # manual sign extension
            push @lines_rb, "val &= ((1 << $retbits) - 1)";
            push @lines_rb, "((val >> ($retbits-1)) & 1) == 0 ? val : val - (1 << $retbits)";
        }

    }
    elsif ($retmeta eq 'pointer')
    {
        # method returns a pointer to some struct, create the correct ruby wrapper
        push @lines_rb, "ptr = $call";
        push @lines_rb, "class << self";
        indent_rb {
            render_item($ret->findnodes('child::ld:item')->[0]);
        };
        push @lines_rb, "end._at(ptr) if ptr != 0";
    }
    else
    {
        print "vmethod unkret $call\n";
        push @lines_rb, $call;
    }
}

sub render_global_objects {
    my (@objects) = @_;
    my @global_objects;

    local $compound_off = 0;
    local $current_typename = 'Global';

    # define all globals as 'fields' of a virtual globalobject wrapping the whole address space
    push @lines_rb, 'class GlobalObjects < MemHack::Compound';
    indent_rb {
        for my $obj (@objects)
        {
            my $oname = $obj->getAttribute('name');

            # check if the symbol is defined in xml to avoid NULL deref
            my $addr = "DFHack.get_global_address('$oname')";
            push @lines_rb, "addr = $addr";
            push @lines_rb, "if addr != 0";
            indent_rb {
                push @lines_rb, "field(:$oname, addr) {";
                my $item = $obj->findnodes('child::ld:item')->[0];
                indent_rb {
                    render_item($item);
                };
                push @lines_rb, "}";
            };
            push @lines_rb, "end";

            push @global_objects, $oname;
        }
    };
    push @lines_rb, "end";

    # define friendlier accessors, eg df.world -> DFHack::GlobalObjects.new._at(0).world
    indent_rb {
        push @lines_rb, "Global = GlobalObjects.new._at(0)";
        for my $oname (@global_objects)
        {
            push @lines_rb, "if DFHack.get_global_address('$oname') != 0";
            indent_rb {
                push @lines_rb, "def self.$oname ; Global.$oname ; end";
                push @lines_rb, "def self.$oname=(v) ; Global.$oname = v ; end";
            };
            push @lines_rb, "end";
        }
    };
}


my %align_cache;
my %sizeof_cache;

sub align_field {
    my ($off, $fldalign) = @_;
    my $dt = $off % $fldalign;
    $off += $fldalign - $dt if $dt > 0;
    return $off;
}

sub get_field_align {
    my ($field) = @_;
    my $al = $SIZEOF_PTR;
    my $meta = $field->getAttribute('ld:meta');

    if ($meta eq 'number') {
        $al = sizeof($field);
        # linux aligns int64_t to $SIZEOF_PTR, windows to 8
        # floats are 4 bytes so no pb
        $al = 4 if ($al > 4 and (($os eq 'linux' and $arch == 32) or $al != 8));
    } elsif ($meta eq 'global') {
        $al = get_global_align($field);
    } elsif ($meta eq 'compound') {
        $al = get_compound_align($field);
    } elsif ($meta eq 'static-array') {
        my $tg = $field->findnodes('child::ld:item')->[0];
        $al = get_field_align($tg);
    } elsif ($meta eq 'bytes') {
        $al = $field->getAttribute('alignment') || 1;
    } elsif ($meta eq 'primitive') {
        my $subtype = $field->getAttribute('ld:subtype');
        if ($subtype eq 'stl-fstream' and $os eq 'windows') { $al = 8; }
    }

    return $al;
}

sub get_global_align {
    my ($field) = @_;

    my $typename = $field->getAttribute('type-name');
    return $align_cache{$typename} if $align_cache{$typename};

    my $g = $global_types{$typename};

    my $st = $field->getAttribute('ld:subtype') || '';
    if ($st eq 'bitfield' or $st eq 'enum' or $g->getAttribute('ld:meta') eq 'bitfield-type')
    {
        my $base = $field->getAttribute('base-type') || $g->getAttribute('base-type') || 'uint32_t';
        print "$st type $base\n" if $base !~ /int(\d+)_t/;
        # dont cache, field->base-type may differ
        return $1/8;
    }

    my $al = 1;
    for my $gf ($g->findnodes('child::ld:field')) {
        my $fld_al = get_field_align($gf);
        $al = $fld_al if $fld_al > $al;
    }
    $align_cache{$typename} = $al;

    return $al;
}

sub get_compound_align {
    my ($field) = @_;

    my $st = $field->getAttribute('ld:subtype') || '';
    if ($st eq 'bitfield' or $st eq 'enum')
    {
        my $base = $field->getAttribute('base-type') || 'uint32_t';
        if ($base eq 'long') {
            return $SIZEOF_LONG;
        }
        print "$st type $base\n" if $base !~ /int(\d+)_t/;
        return $1/8;
    }

    my $al = 1;
    for my $f ($field->findnodes('child::ld:field')) {
        my $fal = get_field_align($f);
        $al = $fal if $fal > $al;
    }

    return $al;
}

sub sizeof {
    my ($field) = @_;
    my $meta = $field->getAttribute('ld:meta');

    if ($meta eq 'number') {
        if ($field->getAttribute('ld:subtype') eq 'long') {
            return $SIZEOF_LONG;
        }

        return $field->getAttribute('ld:bits')/8;

    } elsif ($meta eq 'pointer') {
        return $SIZEOF_PTR;

    } elsif ($meta eq 'static-array') {
        my $count = $field->getAttribute('count');
        my $tg = $field->findnodes('child::ld:item')->[0];
        return $count * sizeof($tg);

    } elsif ($meta eq 'bitfield-type' or $meta eq 'enum-type') {
        my $base = $field->getAttribute('base-type') || 'uint32_t';
        print "$meta type $base\n" if $base !~ /int(\d+)_t/;
        return $1/8;

    } elsif ($meta eq 'global') {
        my $typename = $field->getAttribute('type-name');
        return $sizeof_cache{$typename} if $sizeof_cache{$typename};

        my $g = $global_types{$typename};
        my $st = $field->getAttribute('ld:subtype') || '';
        if ($st eq 'bitfield' or $st eq 'enum' or $g->getAttribute('ld:meta') eq 'bitfield-type')
        {
            my $base = $field->getAttribute('base-type') || $g->getAttribute('base-type') || 'uint32_t';
            print "$st type $base\n" if $base !~ /int(\d+)_t/;
            return $1/8;
        }

        return sizeof($g);

    } elsif ($meta eq 'class-type' or $meta eq 'struct-type' or $meta eq 'compound') {
        return sizeof_compound($field);

    } elsif ($meta eq 'container') {
        my $subtype = $field->getAttribute('ld:subtype');

        if ($subtype eq 'stl-vector') {
            if ($os eq 'linux' or $os eq 'windows') {
                return ($arch == 64) ? 24 : 12;
            } else {
                print "sizeof stl-vector on $os\n";
            }
        } elsif ($subtype eq 'stl-bit-vector') {
            if ($os eq 'linux') {
                return ($arch == 64) ? 40 : 20;
            } elsif ($os eq 'windows') {
                return ($arch == 64) ? 32 : 16;
            } else {
                print "sizeof stl-bit-vector on $os\n";
            }
        } elsif ($subtype eq 'stl-deque') {
            if ($os eq 'linux') {
                return ($arch == 64) ? 80 : 40;
            } elsif ($os eq 'windows') {
                return ($arch == 64) ? 40 : 20;
            } else {
                print "sizeof stl-deque on $os\n";
            }
        } elsif ($subtype eq 'df-linked-list') {
            return 3 * $SIZEOF_PTR;
        } elsif ($subtype eq 'df-flagarray') {
            return 2 * $SIZEOF_PTR;	# XXX length may be 4 on windows?
        } elsif ($subtype eq 'df-static-flagarray') {
            return $field->getAttribute('count');
        } elsif ($subtype eq 'df-array') {
            return 4 + $SIZEOF_PTR;   # XXX 4->2 ?
        } else {
            print "sizeof container $subtype\n";
        }

    } elsif ($meta eq 'primitive') {
        my $subtype = $field->getAttribute('ld:subtype');

        if ($subtype eq 'stl-string') {
            if ($os eq 'linux') {
                return ($arch == 64) ? 8 : 4;
            } elsif ($os eq 'windows') {
                return ($arch == 64) ? 32 : 24;
            } else {
                print "sizeof stl-string on $os\n";
            }
            print "sizeof stl-string\n";
        } elsif ($subtype eq 'stl-fstream') {
            if ($os eq 'linux') {
                return 284; # TODO: fix on x64
            } elsif ($os eq 'windows') {
                return ($arch == 64) ? 280 : 192;
            } else {
                print "sizeof stl-fstream on $os\n";
            }
            print "sizeof stl-fstream\n";
        } else {
            print "sizeof primitive $subtype\n";
        }

    } elsif ($meta eq 'bytes') {
        return $field->getAttribute('size');
    } else {
        print "sizeof $meta\n";
    }
}

sub sizeof_compound {
    my ($field) = @_;

    my $typename = $field->getAttribute('type-name');
    return $sizeof_cache{$typename} if $typename and $sizeof_cache{$typename};

    my $meta = $field->getAttribute('ld:meta');

    my $st = $field->getAttribute('ld:subtype') || '';
    if ($st eq 'bitfield' or $st eq 'enum')
    {
        my $base = $field->getAttribute('base-type') || 'uint32_t';
        if ($base eq 'long') {
            $sizeof_cache{$typename} = $SIZEOF_LONG if $typename;
            return $SIZEOF_LONG;
        }
        print "$st type $base\n" if $base !~ /int(\d+)_t/;
        $sizeof_cache{$typename} = $1/8 if $typename;
        return $1/8;
    }

    if ($field->getAttribute('is-union'))
    {
        my $sz = 0;
        for my $f ($field->findnodes('child::ld:field'))
        {
            my $fsz = sizeof($f);
            $sz = $fsz if $fsz > $sz;
        }
        return $sz;
    }

    my $parent = $field->getAttribute('inherits-from');
    my $off = 0;
    $off = $SIZEOF_PTR if ($meta eq 'class-type');
    $off = sizeof($global_types{$parent}) if ($parent);

    my $al = 1;
    $al = $SIZEOF_PTR if ($meta eq 'class-type');

    for my $f ($field->findnodes('child::ld:field'))
    {
        my $fa = get_field_align($f);
        $al = $fa if $fa > $al;
        $off = align_field($off, $fa);
        $off += sizeof($f);
    }

    # GCC: class a { vtable; char; } ; class b:a { char c2; } -> c2 has offset 5 (Windows MSVC: offset 8)
    $al = 1 if ($meta eq 'class-type' and $os eq 'linux');
    $off = align_field($off, $al);
    $sizeof_cache{$typename} = $off if $typename;

    return $off;
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
    my $subtype = $item->getAttribute('ld:subtype');

    if ($subtype and $subtype eq 'enum') {
        render_item_number($item);
    } else {
        my $rbname = rb_ucase($typename);
        push @lines_rb, "global :$rbname";
    }
}

sub render_item_number {
    my ($item, $classname) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    my $meta = $item->getAttribute('ld:meta');
    my $initvalue = $item->getAttribute('init-value');
    $initvalue ||= -1 if $item->getAttribute('refers-to') or $item->getAttribute('ref-target');
    my $typename = $item->getAttribute('type-name');
    undef $typename if ($meta and $meta eq 'bitfield-type');
    my $g = $global_types{$typename} if ($typename);
    $typename = rb_ucase($typename) if $typename;
    $typename = $classname if (!$typename and $subtype and $subtype eq 'enum');      # compound enum

    $initvalue = 1 if ($initvalue and $initvalue eq 'true');
    $initvalue = ":$initvalue" if ($initvalue and $typename and $initvalue =~ /[a-zA-Z]/);
    $initvalue ||= 'nil' if $typename;

    $subtype = $item->getAttribute('base-type') if (!$subtype or $subtype eq 'bitfield' or $subtype eq 'enum');
    $subtype ||= $g->getAttribute('base-type') if ($g);
    $subtype = 'int32_t' if (!$subtype);

    if ($subtype eq 'uint64_t') {
        push @lines_rb, 'number 64, false';
    } elsif ($subtype eq 'int64_t') {
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
        push @lines_rb, 'number 8, true';
    } elsif ($subtype eq 'bool') {
        push @lines_rb, 'number 8, true';
        $initvalue ||= 'nil';
        $typename ||= 'BooleanEnum';
    } elsif ($subtype eq 'long') {
        push @lines_rb, 'number ' . $SIZEOF_LONG . ', true';
    } elsif ($subtype eq 's-float') {
        push @lines_rb, 'float';
        return;
    } elsif ($subtype eq 'd-float') {
        push @lines_rb, 'double';
        return;
    } else {
        print "no render number $subtype\n";
        return;
    }

    $lines_rb[$#lines_rb] .= ", $initvalue" if ($initvalue);
    $lines_rb[$#lines_rb] .= ", $typename" if ($typename);
}

sub render_item_compound {
    my ($item) = @_;

    my $subtype = $item->getAttribute('ld:subtype');

    local $compound_off = 0;
    my $classname = $current_typename . '_' . rb_ucase($item->getAttribute('ld:typedef-name'));
    local $current_typename = $classname;

    if (!$subtype || $subtype eq 'bitfield')
    {
        push @lines_rb, "compound(:$classname) {";
        indent_rb {
            # declare sizeof() only for useful compound, eg the one behind pointers
            # that the user may want to allocate
            my $sz = sizeof($item);
            push @lines_rb, "sizeof $sz\n" if $compound_pointer;

            if (!$subtype) {
                local $compound_pointer = 0;
                render_struct_fields($item);
            } else {
                render_bitfield_fields($item);
            }
        };
        push @lines_rb, "}"
    }
    elsif ($subtype eq 'enum')
    {
        push @lines_rb, "class ::DFHack::$classname < MemHack::Enum";
        indent_rb {
            # declare constants
            render_enum_fields($item);
        };
        push @lines_rb, "end\n";

        # actual field
        render_item_number($item, $classname);
    }
    else
    {
        print "no render compound $subtype\n";
    }
}

sub render_item_container {
    my ($item) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    my $rbmethod = join('_', split('-', $subtype));
    my $tg = $item->findnodes('child::ld:item')->[0];
    my $indexenum = $item->getAttribute('index-enum');
    my $count = $item->getAttribute('count');
    if ($tg)
    {
        if ($rbmethod eq 'df_linked_list') {
            push @lines_rb, "$rbmethod {";
        } else {
            my $tglen = sizeof($tg) if $tg;
            push @lines_rb, "$rbmethod($tglen) {";
        }
        indent_rb {
            render_item($tg);
        };
        push @lines_rb, "}";
    }
    elsif ($indexenum)
    {
        $indexenum = rb_ucase($indexenum);
        if ($count) {
            push @lines_rb, "$rbmethod($count, $indexenum)";
        } else {
            push @lines_rb, "$rbmethod($indexenum)";
        }
    }
    else
    {
        if ($count) {
            push @lines_rb, "$rbmethod($count)";
        } else {
            push @lines_rb, "$rbmethod";
        }
    }
}

sub render_item_pointer {
    my ($item) = @_;

    my $tg = $item->findnodes('child::ld:item')->[0];
    my $ary = $item->getAttribute('is-array') || '';

    if ($ary eq 'true') {
        my $tglen = sizeof($tg) if $tg;
        push @lines_rb, "pointer_ary($tglen) {";
    } else {
        push @lines_rb, "pointer {";
    }
    indent_rb {
        local $compound_pointer = 1;
        render_item($tg);
    };
    push @lines_rb, "}";
}

sub render_item_staticarray {
    my ($item) = @_;

    my $count = $item->getAttribute('count');
    my $tg = $item->findnodes('child::ld:item')->[0];
    my $tglen = sizeof($tg) if $tg;
    my $indexenum = $item->getAttribute('index-enum');

    if ($indexenum) {
        $indexenum = rb_ucase($indexenum);
        push @lines_rb, "static_array($count, $tglen, $indexenum) {";
    } else {
        push @lines_rb, "static_array($count, $tglen) {";
    }
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
    } elsif ($subtype eq 'stl-fstream') {
    } else {
        print "no render primitive $subtype\n";
    }
}

sub render_item_bytes {
    my ($item) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    if ($subtype eq 'padding') {
    } elsif ($subtype eq 'static-string') {
        my $size = $item->getAttribute('size') || -1;
        push @lines_rb, "static_string($size)";
    } else {
        print "no render bytes $subtype\n";
    }
}



my $input = $ARGV[0] or die "need input xml";
my $output = $ARGV[1] or die "need output file";

my $doc = XML::LibXML->new()->parse_file($input);
$global_types{$_->getAttribute('type-name')} = $_ foreach $doc->findnodes('/ld:data-definition/ld:global-type');

# render enums first, this allows later code to refer to them directly
my @nonenums;
for my $name (sort { $a cmp $b } keys %global_types)
{
    my $type = $global_types{$name};
    my $meta = $type->getAttribute('ld:meta');
    if ($meta eq 'enum-type') {
        render_global_enum($name, $type);
    } else {
        push @nonenums, $name;
    }
}

# render other structs/bitfields/classes
for my $name (@nonenums)
{
    my $type = $global_types{$name};
    my $meta = $type->getAttribute('ld:meta');
    if ($meta eq 'struct-type' or $meta eq 'class-type') {
        render_global_class($name, $type);
    } elsif ($meta eq 'bitfield-type') {
        render_global_bitfield($name, $type);
    } else {
        print "no render global type $meta\n";
    }
}

# render globals
render_global_objects($doc->findnodes('/ld:data-definition/ld:global-object'));


open FH, ">$output";
print FH "module DFHack\n";
print FH "$_\n" for @lines_rb;
print FH "end\n";
close FH;
