#!/usr/bin/perl

use strict;
use warnings;

use XML::LibXML;

our @lines_rb;
my @lines_cpp;
my @include_cpp;
my %offsets;

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


my %seen_enum_name;
sub render_global_enum {
    my ($name, $type) = @_;

    my $rbname = rb_ucase($name);
    push @lines_rb, "class $rbname";
    %seen_enum_name = ();
    indent_rb {
        render_enum_fields($type);
    };
    push @lines_rb, "end\n";
}
sub render_enum_fields {
    my ($type) = @_;

    my $value = -1;
    my $idxname = 'ENUM';
    $idxname .= '_' while ($seen_enum_name{$idxname});
    $seen_enum_name{$idxname}++;
    push @lines_rb, "$idxname = {}";
    for my $item ($type->findnodes('child::enum-item')) {
        $value = $item->getAttribute('value') || ($value+1);
        my $elemname = $item->getAttribute('name'); # || "unk_$value";

        if ($elemname) {
            my $rbelemname = rb_ucase($elemname);
            $rbelemname .= '_' while ($seen_enum_name{$rbelemname});
            $seen_enum_name{$rbelemname}++;
            push @lines_rb, "$rbelemname = $value ; ${idxname}[$value] = :$rbelemname";
        }
    }
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
        render_item_number($type, '');
    };
    push @lines_rb, "}";

    my $shift = 0;
    for my $field ($type->findnodes('child::ld:field')) {
        my $count = $field->getAttribute('count') || 1;
        my $name = $field->getAttribute('name');
        $name = $field->getAttribute('ld:anon-name') if (!$name);
        print "bitfield $name !number\n" if (!($field->getAttribute('ld:meta') eq 'number'));
        if ($count == 1) {
            push @lines_rb, "field(:$name, 0) { bit $shift }" if ($name);
        } else {
            push @lines_rb, "field(:$name, 0) { bits $shift, $count }" if ($name);
        }
        $shift += $count;
    }
}


my %global_types;
my $cpp_var_counter = 0;
my %seen_class;
sub render_global_class {
    my ($name, $type) = @_;

    my $rbname = rb_ucase($name);

    # ensure pre-definition of ancestors
    my $parent = $type->getAttribute('inherits-from');
    render_global_class($parent, $global_types{$parent}) if ($parent and !$seen_class{$parent});

    return if $seen_class{$name};
    $seen_class{$name}++;
    %seen_enum_name = ();

    my $rtti_name = $type->getAttribute('original-name') ||
                    $type->getAttribute('type-name');

    my $has_rtti = $parent;
    if (!$parent and $type->getAttribute('ld:meta') eq 'class-type') {
        for my $anytypename (keys %global_types) {
            my $anytype = $global_types{$anytypename};
            if ($anytype->getAttribute('ld:meta') eq 'class-type') {
                my $anyparent = $anytype->getAttribute('inherits-from');
                $has_rtti = 1 if ($anyparent and $anyparent eq $name);
            }
        }
    }

    my $rbparent = ($parent ? rb_ucase($parent) : 'MemHack::Compound');

    my $cppns = "df::$name"; 
    push @lines_cpp, "}" if @include_cpp;
    push @lines_cpp, "void cpp_$name(FILE *fout) {";
    push @include_cpp, $name;

    push @lines_rb, "class $rbname < $rbparent";
    indent_rb {
        my $sz = query_cpp("sizeof($cppns)");
        push @lines_rb, "sizeof $sz";
        push @lines_rb, "rtti_classname '$rtti_name'" if $has_rtti;
        render_struct_fields($type, "$cppns");
    };
    push @lines_rb, "end\n";
}
sub render_struct_fields {
    my ($type, $cppns) = @_;

    for my $field ($type->findnodes('child::ld:field')) {
        my $name = $field->getAttribute('name');
        $name = $field->getAttribute('ld:anon-name') if (!$name);
        if (!$name and $field->getAttribute('ld:anon-compound')) {
            render_struct_fields($field, $cppns);
        }
        next if (!$name);
        my $offset = get_offset($cppns, $name);

        push @lines_rb, "field(:$name, $offset) {";
        indent_rb {
            render_item($field, "$cppns");
        };
        push @lines_rb, "}";
    }
}

sub render_global_objects {
    my (@objects) = @_;
    my @global_objects;

    my $sname = 'global_objects';
    my $rbname = rb_ucase($sname);

    %seen_enum_name = ();

    push @lines_cpp, "}" if @include_cpp;
    push @lines_cpp, "void cpp_$sname(FILE *fout) {";
    push @include_cpp, $sname;

    push @lines_rb, "class $rbname < MemHack::Compound";
    indent_rb {
        for my $obj (@objects) {
            my $oname = $obj->getAttribute('name');
            my $addr = "DFHack.get_global_address('$oname')";
            push @lines_rb, "addr = $addr";
            push @lines_rb, "if addr != 0";
            indent_rb {
                push @lines_rb, "field(:$oname, addr) {";
                my $item = $obj->findnodes('child::ld:item')->[0];
                indent_rb {
                    render_item($item, 'df::global');
                };
                push @lines_rb, "}";
            };
            push @lines_rb, "end";

            push @global_objects, $oname;
        }
    };
    push @lines_rb, "end";

    indent_rb {
        push @lines_rb, "Global = GlobalObjects.new._at(0)";
        for my $obj (@global_objects) {
            push @lines_rb, "def self.$obj ; Global.$obj ; end";
            push @lines_rb, "def self.$obj=(v) ; Global.$obj = v ; end";
        }
    };
}


sub render_item {
    my ($item, $pns) = @_;
    return if (!$item);

    my $meta = $item->getAttribute('ld:meta');

    my $renderer = $item_renderer{$meta};
    if ($renderer) {
        $renderer->($item, $pns);
    } else {
        print "no render item $meta\n";
    }
}

sub render_item_global {
    my ($item, $pns) = @_;

    my $typename = $item->getAttribute('type-name');
    my $subtype = $item->getAttribute('ld:subtype');

    if ($subtype and $subtype eq 'enum') {
        render_item_number($item, $pns);
    } else {
        my $rbname = rb_ucase($typename);
        push @lines_rb, "global :$rbname";
    }
}

sub render_item_number {
    my ($item, $pns) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    $subtype = $item->getAttribute('base-type') if (!$subtype or $subtype eq 'enum' or $subtype eq 'bitfield');
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
    my ($item, $pns) = @_;

    my $cppns = $pns . '::' . $item->getAttribute('ld:typedef-name');
    my $subtype = $item->getAttribute('ld:subtype');
    if (!$subtype || $subtype eq 'bitfield') {
        push @lines_rb, "compound {";
        indent_rb {
            if (!$subtype) {
                render_struct_fields($item, $cppns);
            } else {
                render_bitfield_fields($item);
            }
        };
        push @lines_rb, "}"
    } elsif ($subtype eq 'enum') {
        # declare constants
        render_enum_fields($item);
        # actual field
        render_item_number($item, $cppns);
    } else {
        print "no render compound $subtype\n";
    }
}

sub render_item_container {
    my ($item, $pns) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    my $rbmethod = join('_', split('-', $subtype));
    my $tg = $item->findnodes('child::ld:item')->[0];
    my $indexenum = $item->getAttribute('index-enum');
    if ($tg) {
        if ($rbmethod eq 'df_linked_list') {
            push @lines_rb, "$rbmethod {";
        } else {
            my $tglen = get_tglen($tg, $pns);
            push @lines_rb, "$rbmethod($tglen) {";
        }
        indent_rb {
            render_item($tg, $pns);
        };
        push @lines_rb, "}";
    } elsif ($indexenum) {
        $indexenum = rb_ucase($indexenum);
        push @lines_rb, "$rbmethod(:$indexenum)";
    } else {
        push @lines_rb, "$rbmethod";
    }
}

sub render_item_pointer {
    my ($item, $pns) = @_;

    my $tg = $item->findnodes('child::ld:item')->[0];
    my $ary = $item->getAttribute('is-array');
    if ($ary and $ary eq 'true') {
        my $tglen = get_tglen($tg, $pns);
        push @lines_rb, "pointer_ary($tglen) {";
    } else {
        push @lines_rb, "pointer {";
    }
    indent_rb {
        render_item($tg, $pns);
    };
    push @lines_rb, "}";
}

sub render_item_staticarray {
    my ($item, $pns) = @_;

    my $count = $item->getAttribute('count');
    my $tg = $item->findnodes('child::ld:item')->[0];
    my $tglen = get_tglen($tg, $pns);
    my $indexenum = $item->getAttribute('index-enum');
    if ($indexenum) {
        $indexenum = rb_ucase($indexenum);
        push @lines_rb, "static_array($count, $tglen, :$indexenum) {";
    } else {
        push @lines_rb, "static_array($count, $tglen) {";
    }
    indent_rb {
        render_item($tg, $pns);
    };
    push @lines_rb, "}";
}

sub render_item_primitive {
    my ($item, $pns) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    if ($subtype eq 'stl-string') {
        push @lines_rb, "stl_string";
    } else {
        print "no render primitive $subtype\n";
    }
}

sub render_item_bytes {
    my ($item, $pns) = @_;

    my $subtype = $item->getAttribute('ld:subtype');
    if ($subtype eq 'padding') {
    } elsif ($subtype eq 'static-string') {
        my $size = $item->getAttribute('size');
        push @lines_rb, "static_string($size)";
    } else {
        print "no render bytes $subtype\n";
    }
}

sub get_offset {
    my ($cppns, $fname) = @_;

    return query_cpp("offsetof($cppns, $fname)");
}

sub get_tglen {
    my ($tg, $cppns) = @_;

    if (!$tg) {
        return 'nil';
    }

    my $meta = $tg->getAttribute('ld:meta');
    if ($meta eq 'number') {
        return $tg->getAttribute('ld:bits')/8;
    } elsif ($meta eq 'pointer') {
        return 4;
    } elsif ($meta eq 'container') {
        my $subtype = $tg->getAttribute('ld:subtype');
        if ($subtype eq 'stl-vector') {
            return query_cpp("sizeof(std::vector<int>)");
        } elsif ($subtype eq 'df-linked-list') {
            return 12;
        } else {
            print "cannot tglen container $subtype\n";
        }
    } elsif ($meta eq 'compound') {
        my $cname = $tg->getAttribute('ld:typedef-name');
        return query_cpp("sizeof(${cppns}::$cname)");
    } elsif ($meta eq 'static-array') {
        my $count = $tg->getAttribute('count');
        my $ttg = $tg->findnodes('child::ld:item')->[0];
        my $ttgl = get_tglen($ttg, $cppns);
        if ($ttgl =~ /^\d+$/) {
            return $count * $ttgl;
        } else {
            return "$count*$ttgl";
        }
    } elsif ($meta eq 'global') {
        my $typename = $tg->getAttribute('type-name');
        my $subtype = $tg->getAttribute('ld:subtype');
        if ($subtype and $subtype eq 'enum') {
            my $base = $tg->getAttribute('base-type') || 'int32_t';
            if ($base eq 'int32_t') {
                return 4;
            } elsif ($base eq 'int16_t') {
                return 2;
            } elsif ($base eq 'int8_t') {
                return 1;
            } else {
                print "cannot tglen enum $base\n";
            }
        } else {
            return query_cpp("sizeof(df::$typename)");
        }
    } elsif ($meta eq 'primitive') {
        my $subtype = $tg->getAttribute('ld:subtype');
        if ($subtype eq 'stl-string') {
            return query_cpp("sizeof(std::string)");
        } else {
            print "cannot tglen primitive $subtype\n";
        }
    } else {
        print "cannot tglen $meta\n";
    }
}

my %query_cpp_cache;
sub query_cpp {
    my ($query) = @_;

    my $ans = $offsets{$query};
    return $ans if (defined($ans));

    my $cached = $query_cpp_cache{$query};
    return $cached if (defined($cached));
    $query_cpp_cache{$query} = 1;

    push @lines_cpp, "    fprintf(fout, \"%s = %d\\n\", \"$query\", $query);";
    return "'$query'";
}



my $input = $ARGV[0] || '../../library/include/df/codegen.out.xml';

# run once with output = 'ruby-autogen.cpp'
# compile
# execute, save output to 'ruby-autogen.offsets'
# re-run this script with output = 'ruby-autogen.rb' and offsetfile = 'ruby-autogen.offsets'
# delete binary
# delete offsets
my $output = $ARGV[1] or die "need output file";
my $offsetfile = $ARGV[2];
my $memstruct = $ARGV[3];

if ($offsetfile) {
    open OF, "<$offsetfile";
    while (my $line = <OF>) {
        chomp($line);
        my ($key, $val) = split(' = ', $line);
        $offsets{$key} = $val;
    }
    close OF;
}


my $doc = XML::LibXML->new()->parse_file($input);
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


render_global_objects($doc->findnodes('/ld:data-definition/ld:global-object'));


open FH, ">$output";
if ($output =~ /\.cpp$/) {
    print FH "#include \"DataDefs.h\"\n";
    print FH "#include \"df/$_.h\"\n" for @include_cpp;
    print FH "#include <stdio.h>\n";
    print FH "#include <stddef.h>\n";
    print FH "$_\n" for @lines_cpp;
    print FH "}\n";
    print FH "int main(int argc, char **argv) {\n";
    print FH "    FILE *fout;\n";
    print FH "    if (argc < 2) return 1;\n";
    print FH "    fout = fopen(argv[1], \"w\");\n";
    print FH "    cpp_$_(fout);\n" for @include_cpp;
    print FH "    fclose(fout);\n";
    print FH "    return 0;\n";
    print FH "}\n";

} else {
    if ($memstruct) {
        open MH, "<$memstruct";
        print FH "$_" while(<MH>);
        close MH;
    }
    print FH "module DFHack\n";
    print FH "$_\n" for @lines_rb;
    print FH "end\n";
}
close FH;
