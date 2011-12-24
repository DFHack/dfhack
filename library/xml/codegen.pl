#!/usr/bin/perl

use strict;
use warnings;
   
use XML::LibXML;

my $input_dir = $ARGV[0] || '.';
my $output_dir = $ARGV[1] || 'codegen';
my $main_namespace = $ARGV[2] || 'df';
my $export_prefix = 'DFHACK_EXPORT ';

my %types;
my %type_files;

# Misc XML analysis

our $typename;
our $filename;

sub parse_address($;$) {
    my ($str,$in_bits) = @_;
    return undef unless defined $str;
    
    # Parse the format used by offset attributes in xml
    $str =~ /^0x([0-9a-f]+)(?:\.([0-7]))?$/
        or die "Invalid address syntax: $str\n";
    my ($full, $bv) = ($1, $2);
    die "Bits not allowed: $str\n" unless $in_bits;
    return $in_bits ? (hex($full)*8 + ($bv||0)) : hex($full);
}

sub check_bad_attrs($;$$) {
    my ($tag, $allow_size, $allow_align) = @_;
    
    die "Cannot use size, alignment or offset for ".$tag->nodeName."\n"
        if ((!$allow_size && defined $tag->getAttribute('size')) ||
            defined $tag->getAttribute('offset') ||
            (!$allow_align && defined $tag->getAttribute('alignment')));
}

sub check_name($) {
    my ($name) = @_;
    $name =~ /^[_a-zA-Z][_a-zA-Z0-9]*$/
        or die "Invalid identifier: $name\n";
    return $name;
}

sub is_attr_true($$) {
    my ($tag, $name) = @_;
    return ($tag->getAttribute($name)||'') eq 'true';
}

sub type_header_def($) {
    my ($name) = @_;
    return uc($main_namespace).'_'.uc($name).'_H';
}

# Text generation with indentation

our @lines;
our $indentation = 0;

sub with_emit(&;$) { 
    # Executes the code block, and returns emitted lines
    my ($blk, $start_indent) = @_;
    local @lines;
    local $indentation = ($start_indent||0);
    $blk->();
    return @lines;
}

sub emit(@) {
    # Emit an indented line to be returned from with_emit
    my $line = join('',map { defined($_) ? $_ : '' } @_);
    $line = (' 'x$indentation).$line unless length($line) == 0;
    push @lines, $line;
}

sub indent(&) {
    # Indent lines emitted from the block by one step
    my ($blk) = @_;
    local $indentation = $indentation+2;
    $blk->();
}

sub outdent(&) {
    # Unindent lines emitted from the block by one step
    my ($blk) = @_;
    local $indentation = ($indentation >= 2 ? $indentation-2 : 0);
    $blk->();
}

sub emit_block(&;$$) {
    # Emit a full {...} block with indentation
    my ($blk, $prefix, $suffix) = @_;
    $prefix ||= '';
    $suffix ||= '';
    emit $prefix,'{';
    &indent($blk);
    emit '}',$suffix;
}

# Static file output

my @static_lines;
my %static_includes;

sub with_emit_static(&) {
    my ($blk) = @_;
    $static_includes{$typename}++;
    push @static_lines, &with_emit($blk,2);
}

# Anonymous variable names

our $anon_id = 0;
our $anon_prefix;

sub ensure_name($) {
    # If the name is empty, assign an auto-generated one
    my ($name) = @_;
    unless ($name) {
        $name = $anon_prefix.(($anon_id == 0) ? '' : '_'.$anon_id);
        $anon_id++;
    }
    return check_name($name);
}

sub with_anon(&;$) {
    # Establish a new anonymous namespace
    my ($blk,$stem) = @_;
    local $anon_id = $stem ? 0 : 1;
    local $anon_prefix = ($stem||'anon');
    $blk->();
}

# Primitive types

my @primitive_type_list =
    qw(int8_t uint8_t int16_t uint16_t
       int32_t uint32_t int64_t uint64_t
       s-float
       bool ptr-string stl-string flag-bit
       pointer);

my %primitive_aliases = (
    'stl-string' => 'std::string',
    'ptr-string' => 'char*',
    'flag-bit' => 'void',
    'pointer' => 'void*',
    's-float' => 'float',
);

my %primitive_types;
$primitive_types{$_}++ for @primitive_type_list;

sub primitive_type_name($) {
    my ($tag_name) = @_;
    $primitive_types{$tag_name}
        or die "Not primitive: $tag_name\n";
    return $primitive_aliases{$tag_name} || $tag_name;
}

# Type references

our %weak_refs;
our %strong_refs;

sub register_ref($;$) {
    # Register a reference to another type.
    # Strong ones require the type to be included.
    my ($ref, $is_strong) = @_;

    if ($ref) {
        my $type = $types{$ref}
            or die "Unknown type $ref referenced.\n";

        if ($is_strong) {
            $strong_refs{$ref}++;
        } else {
            $weak_refs{$ref}++;
        }
    }
}

# Determines if referenced other types should be included or forward-declared
our $is_strong_ref = 1;

sub with_struct_block(&$;$%) {
    my ($blk, $tag, $name, %flags) = @_;
    
    my $kwd = (is_attr_true($tag,'is-union') ? "union" : "struct");
    my $exp = $flags{-export} ? $export_prefix : '';
    my $prefix = $kwd.' '.$exp.($name ? $name.' ' : '');
    
    emit_block {
        local $_;
        local $is_strong_ref = 1; # reset the state
        if ($flags{-no_anon}) {
            $blk->();
        } else {
            &with_anon($blk);
        }
    } $prefix, ";";
}

sub decode_type_name_ref($;%) {
    # Interpret the type-name field of a tag
    my ($tag,%flags) = @_;
    my $force_type = $flags{-force_type};
    my $force_strong = $flags{-force_strong};
    my $tname = $tag->getAttribute($flags{-attr_name} || 'type-name')
        or return undef;

    if ($primitive_types{$tname}) {
        die "Cannot use type $tname as type-name of ".$tag->nodeName."\n"
            if ($force_type && $force_type ne 'primitive');
        return primitive_type_name($tname);
    } else {
        register_ref $tname, ($force_strong||$is_strong_ref);
        die "Cannot use type $tname as type-name of ".$tag->nodeName."\n"
            if ($force_type && $force_type ne $types{$tname}->nodeName);
        return $main_namespace.'::'.$tname;
    }
}

# CONDITIONALS

sub is_conditional($) {
    my ($tag) = @_;
    return $tag->nodeName =~ /^(cond-if|cond-elseif)$/;    
}

sub translate_if_cond($) {
    my ($tag) = @_;

    my @rules;
    if (my $defvar = $tag->getAttribute('defined')) {
        push @rules, "defined($defvar)";
    }
    if (my $cmpvar = $tag->getAttribute('var')) {
        if (my $cmpval = $tag->getAttribute('lt')) {
            push @rules, "($cmpvar < $cmpval)";
        }
        if (my $cmpval = $tag->getAttribute('le')) {
            push @rules, "($cmpvar <= $cmpval)";
        }
        if (my $cmpval = $tag->getAttribute('eq')) {
            push @rules, "($cmpvar == $cmpval)";
        }
        if (my $cmpval = $tag->getAttribute('ge')) {
            push @rules, "($cmpvar >= $cmpval)";
        }
        if (my $cmpval = $tag->getAttribute('gt')) {
            push @rules, "($cmpvar > $cmpval)";
        }
        if (my $cmpval = $tag->getAttribute('ne')) {
            push @rules, "($cmpvar != $cmpval)";
        }
    }
    return '('.(join(' && ',@rules) || '1').')';
}

our $in_cond = 0;

sub render_cond_if($$$;@) {
    my ($tag, $in_elseif, $render_cb, @tail) = @_;

    local $in_cond = 1;

    {
        local $indentation = 0;
        my $op = ($in_elseif && $in_elseif >= 2) ? '#elif' : '#if';
        emit $op, ' ', translate_if_cond($tag);
    }

    for my $child ($tag->findnodes('child::*')) {
        &render_cond($child, $render_cb, @tail);
    }

    unless ($in_elseif) {
        local $indentation = 0;
        emit "#endif";
    }
}

sub render_cond($$;@) {
    my ($tag, $render_cb, @tail) = @_;
    
    my $tag_name = $tag->nodeName;
    if ($tag_name eq 'cond-if') {
        render_cond_if($tag, 0, $render_cb, @tail);
    } elsif ($tag_name eq 'cond-elseif') {
        my $idx = 1;
        for my $child ($tag->findnodes('child::*')) {
            ($child->nodeName eq 'cond-if')
                or die "Only cond-if tags may be inside a cond-switch: ".$child->nodeName."\n";
            render_cond_if($child, $idx++, $render_cb, @tail);
        }
        {
            local $indentation = 0;
            emit "#endif";
        }
    } else {
        local $_ = $tag;
        $render_cb->($tag, @tail);
    }
}

# ENUM

sub render_enum_core($$) {
    my ($name,$tag) = @_;

    my $base = 0;

    emit_block {
        my @items = $tag->findnodes('child::*');
        my $idx = 0;

        for my $item (@items) {
            render_cond $item, sub {
                my $tag = $_->nodeName;
                return if $tag eq 'enum-attr';
                ($tag eq 'enum-item')
                    or die "Invalid enum member: ".$item->nodeName."\n";

                my $name = ensure_name $_->getAttribute('name');
                my $value = $_->getAttribute('value');

                $base = ($idx == 0 && !$in_cond) ? $value : undef if defined $value;
                $idx++;

                emit $name, (defined($value) ? ' = '.$value : ''), ',';
            };
        }

        emit "_last_item_of_$name";
    } "enum $name ", ";";

    return $base;
}

sub render_enum_tables($$$) {
    my ($name,$tag,$base) = @_;

    # Enumerate enum attributes

    my %aidx = ('key' => 0);
    my @anames = ('key');
    my @avals = ('NULL');
    my @atypes = ('const char*');
    my @atnames = (undef);

    for my $attr ($tag->findnodes('child::enum-attr')) {
        my $name = $attr->getAttribute('name') or die "Unnamed enum-attr.\n";
        my $type = $attr->getAttribute('type-name');
        my $def = $attr->getAttribute('default-value');

        die "Duplicate attribute $name.\n" if exists $aidx{$name};

        check_name $name;
        $aidx{$name} = scalar @anames;
        push @anames, $name;
        push @atnames, $type;

        if ($type) {
            push @atypes, $type;
            push @avals, (defined $def ? $def : "($type)0");
        } else {
            push @atypes, 'const char*';
            push @avals, (defined $def ? "\"$def\"" : 'NULL');
        }
    }

    # Emit accessor function prototypes

    emit "const $name _first_item_of_$name = ($name)$base;";

    emit_block {
        emit "return (value >= _first_item_of_$name && value < _last_item_of_$name);";
    } "inline bool is_valid($name value) ";

    for (my $i = 0; $i < @anames; $i++) {
        emit "${export_prefix}$atypes[$i] get_$anames[$i]($name value);";
    }

    # Emit implementation

    with_emit_static {
        emit_block {
            emit_block {
                # Emit the entry type
                emit_block {
                    for (my $i = 0; $i < @anames; $i++) {
                        emit "$atypes[$i] $anames[$i];";
                    }
                } "struct _info_entry ", ";";
            
                # Emit the info table
                emit_block {
                    for my $item ($tag->findnodes('child::*')) {
                        render_cond $item, sub {
                            my $tag = $_->nodeName;
                            return if $tag eq 'enum-attr';
                            
                            # Assemble item-specific attr values
                            my @evals = @avals;
                            my $name = $_->getAttribute('name');
                            $evals[0] = "\"$name\"" if $name;

                            for my $attr ($_->findnodes('child::item-attr')) {
                                my $name = $attr->getAttribute('name') or die "Unnamed item-attr.\n";
                                my $value = $attr->getAttribute('value') or die "No-value item-attr.\n";
                                my $idx = $aidx{$name} or die "Unknown item-attr: $name\n";

                                if ($atnames[$idx]) {
                                    $evals[$idx] = $value;
                                } else {
                                    $evals[$idx] = "\"$value\"";
                                }
                            }

                            emit "{ ",join(', ',@evals)," },";
                        };
                    }

                    emit "{ ",join(', ',@avals)," }";
                } "static const _info_entry _info[] = ", ";";

                for (my $i = 0; $i < @anames; $i++) {
                    emit_block {
                        emit "return is_valid(value) ? _info[value - $base].$anames[$i] : $avals[$i];";
                    } "$atypes[$i] get_$anames[$i]($name value) ";
                }
            } "namespace $name ";
        } "namespace enums ";
    };
}

sub render_enum_type {
    my ($tag) = @_;

    emit_block {
        emit_block {
            my $base = render_enum_core($typename,$tag);
            
            if (defined $base) {
                render_enum_tables($typename,$tag,$base);
            } else {
                print STDERR "Warning: complex enum: $typename\n";
            }
        } "namespace $typename ";
    } "namespace enums ";

    emit "using enums::",$typename,"::",$typename,";";
}

# BITFIELD

sub get_primitive_base($;$) {
    my ($tag, $default) = @_;

    my $base = $tag->getAttribute('base-type') || $default || 'uint32_t';
    $primitive_types{$base} or die "Must be primitive: $base\n";

    return $base;
}

sub render_bitfield_core {
    my ($name, $tag) = @_;

    emit_block {
        emit get_primitive_base($tag), ' whole;';

        emit_block {
            for my $item ($tag->findnodes('child::*')) {
                render_cond $item, sub {
                    my ($item) = @_;
                    ($item->nodeName eq 'flag-bit')
                        or die "Invalid bitfield member:".$item->nodeName."\n";

                    check_bad_attrs($item);
                    my $name = ensure_name $item->getAttribute('name');
                    my $size = $item->getAttribute('count') || 1;
                    emit "unsigned ", $name, " : ", $size, ";";
                };
            }
        } "struct ", " bits;";
    } "union $name ", ";";
}

sub render_bitfield_type {
    my ($tag) = @_;
    render_bitfield_core($typename,$tag);
}

# STRUCT

my %struct_field_handlers;

sub get_struct_fields($) {
    # Retrieve subtags that are actual struct fields
    my ($struct_tag) = @_;
    local $_;
    return grep {
        my $tag = $_->nodeName;
        die "Unknown field tag: $tag\n"
            unless exists $struct_field_handlers{$tag};
        $struct_field_handlers{$tag};
    } $struct_tag->findnodes('child::*');
}

sub get_struct_field_type($) {
    # Dispatch on the tag name, and retrieve the type prefix & suffix
    my ($tag) = @_;
    my $handler = $struct_field_handlers{$tag->nodeName}
        or die "Unexpected tag: ".$tag->nodeName;
    return $handler->($tag);
}

sub do_render_struct_field($) {
    my ($tag) = @_;
    my $tag_name = $tag->nodeName;
    my $field_name = $tag->getAttribute('name');

    # Special case: anonymous compounds.
    if ($tag_name eq 'compound' && !defined $field_name &&
        !defined $tag->getAttribute('type-name')) 
    {
        check_bad_attrs($tag);
        with_struct_block {
            render_struct_field($_) for get_struct_fields($tag);
        } $tag, undef, -no_anon => 1;
        return;
    }

    # Otherwise, create the name if necessary, and render
    my $name = ensure_name $field_name;
    with_anon {
        my ($prefix, $postfix) = get_struct_field_type($tag);
        emit $prefix, ' ', $name, $postfix, ';';
    } "T_$name";
}

sub render_struct_field($) {
    my ($tag) = @_;
    render_cond $tag, \&do_render_struct_field;
}

sub emit_typedef($$) {
    # Convert a prefix/postfix pair into a single name
    my ($pre, $post) = @_;
    my $name = ensure_name undef;
    emit 'typedef ', $pre, ' ', $name, $post, ';';
    return $name;
}

sub get_container_item_type($$;$) {
    # Interpret the type-name and nested fields for a generic container type
    my ($tag,$strong_ref,$allow_void) = @_;
    
    check_bad_attrs($tag);

    my $prefix;
    my $postfix = '';
    local $is_strong_ref = $strong_ref;

    unless ($prefix = decode_type_name_ref($tag)) {
        my @fields = get_struct_fields($tag);

        if (scalar(@fields) == 1 && !is_conditional($fields[0])) {
            ($prefix, $postfix) = get_struct_field_type($fields[0]);
        } elsif (scalar(@fields) == 0) {
            $allow_void or die "Empty container: ".$tag->nodeName."\n";
            $prefix = $allow_void;            
        } else {
            $prefix = ensure_name undef;
            with_struct_block {
                render_struct_field($_) for @fields;
            } $tag, $prefix;
        }
    }
    
    return ($prefix,$postfix) if wantarray;
    return emit_typedef($prefix, $postfix) if $postfix;
    return $prefix;
}

sub get_primitive_field_type {
    # Primitive type handler
    my ($tag,$fname) = @_;
    check_bad_attrs($tag);
    my $name = $tag->nodeName;
    return (primitive_type_name($name), "");
}

sub get_static_string_type {
    # Static string handler
    my ($tag, $fname) = @_;
    check_bad_attrs($tag, 1);
    my $count = $tag->getAttribute('size') || 0;
    return ('char', "[$count]");
}

sub get_padding_type {
    # Padding handler. Supports limited alignment.
    my ($tag, $fname) = @_;

    check_bad_attrs($tag, 1, 1);
    my $count = $tag->getAttribute('size') || 0;
    my $align = $tag->getAttribute('alignment') || 1;

    if ($align == 1) {
        return ('char', "[$count]");
    } elsif ($align == 2) {
        ($count % 2 == 0) or die "Size not aligned in padding: $count at $align\n";
        return ('short', "[".($count/2)."]");
    } elsif ($align == 4) {
        ($count % 4 == 0) or die "Size not aligned in padding: $count at $align\n";
        return ('int', "[".($count/4)."]");
    } else {
        die "Bad padding alignment $align in $typename in $filename\n";
    }
}

sub get_static_array_type {
    # static-array handler
    my ($tag, $fname) = @_;
    my ($pre, $post) = get_container_item_type($tag, 1);
    my $count = $tag->getAttribute('count')
        or die "Count is mandatory for static-array in $typename in $filename\n";
    return ($pre, "[$count]".$post);
}

sub get_pointer_type($) {
    # pointer handler
    my ($tag) = @_;
    my $item = get_container_item_type($tag, 0, 'void');
    return ($item.'*', '');
}

sub get_compound_type($) {
    # compound (nested struct) handler
    my ($tag) = @_;
    check_bad_attrs($tag);

    my $tname = decode_type_name_ref($tag);
    unless ($tname) {
        $tname = ensure_name undef;
        with_struct_block {
            render_struct_field($_) for get_struct_fields($tag);
        } $tag, $tname;
    }
    return ($tname,'');
}

sub get_bitfield_type($) {
    # nested bitfield handler
    my ($tag) = @_;
    check_bad_attrs($tag);

    my $tname = decode_type_name_ref($tag, -force_type => 'bitfield-type');
    unless ($tname) {
        $tname = ensure_name undef;
        with_anon {
            render_bitfield_core($tname, $tag);
        };
    }
    return ($tname,'');
}

sub get_enum_type($) {
    # nested enum handler
    my ($tag) = @_;
    check_bad_attrs($tag);

    my $tname = decode_type_name_ref($tag, -force_type => 'enum-type', -force_strong => 1);
    my $base = get_primitive_base($tag, 'int32_t');
    unless ($tname) {
        $tname = ensure_name undef;
        with_anon {
            render_enum_core($tname,$tag);
        };
    }
    return ("enum_field<$tname,$base>", '');
}

sub get_stl_vector_type($) {
    # STL vector
    my ($tag) = @_;
    my $item = get_container_item_type($tag,1,'void*');
    $item = 'char' if $item eq 'bool';
    return ("std::vector<$item>", '');
}

sub get_stl_bit_vector_type($) {
    # STL bit vector
    my ($tag) = @_;
    check_bad_attrs($tag);
    return ("std::vector<bool>", '');
}

sub get_df_flagarray_type($) {
    # DF flag array
    my ($tag) = @_;
    check_bad_attrs($tag);
    my $type = decode_type_name_ref($tag, -attr_name => 'index-enum', -force_type => 'enum-type', -force_strong => 1) || 'int';
    return ("BitArray<$type>", '');
}

# Struct dispatch table and core

%struct_field_handlers = (
    'comment' => undef, # skip
    'code-helper' => undef, # skip
    'cond-if' => sub { die "cond handling error"; },
    'cond-elseif' => sub { die "cond handling error"; },
    'static-string' => \&get_static_string_type,
    'padding' => \&get_padding_type,
    'static-array' => \&get_static_array_type,
    'pointer' => \&get_pointer_type,
    'compound' => \&get_compound_type,
    'bitfield' => \&get_bitfield_type,
    'enum' => \&get_enum_type,
    'stl-vector' => \&get_stl_vector_type,
    'stl-bit-vector' => \&get_stl_bit_vector_type,
    'df-flagarray' => \&get_df_flagarray_type,
);
$struct_field_handlers{$_} ||= \&get_primitive_field_type for @primitive_type_list;

sub render_struct_type {
    my ($tag) = @_;

    my $tag_name = $tag->nodeName;
    my $is_class = ($tag_name eq 'class-type');
    my $has_methods = $is_class || is_attr_true($tag, 'has-methods');
    my $inherits = $tag->getAttribute('inherits-from');
    my $original_name = $tag->getAttribute('original-name');
    my $ispec = '';

    if ($inherits) {
        register_ref $inherits, 1;
        $ispec = ' : '.$inherits;
    } elsif ($is_class) {
        $ispec = ' : virtual_class';
    }

    with_struct_block {
        render_struct_field($_) for get_struct_fields($tag);

        if ($has_methods) {
            if ($is_class) {
                emit "static class_virtual_identity<$typename> _identity;";
                with_emit_static {
                    emit "class_virtual_identity<$typename> ${typename}::_identity(",
                         "\"$typename\",",
                         ($original_name ? "\"$original_name\"" : 'NULL'), ',',
                         ($inherits ? "&${inherits}::_identity" : 'NULL'),
                         ");";
                }
            }

            outdent {
                emit "protected:";
            };

            if ($is_class) {
                emit "virtual ~",$typename,"() {}";
            } else {
                emit "~",$typename,"() {}";
            }
        }
    } $tag, "$typename$ispec", -export => 1;
}

# MAIN BODY

# Collect all type definitions from XML files

sub add_type_to_hash($) {
    my ($type) = @_;

    my $name = $type->getAttribute('type-name')
        or die "Type without a name in $filename\n";

    die "Duplicate definition of $name in $filename\n" if $types{$name};

    local $typename = $name;
    check_bad_attrs $type;
    $types{$name} = $type;
    $type_files{$name} = $filename;
}

for my $fn (glob "$input_dir/*.xml") {
    local $filename = $fn;
    my $parser = XML::LibXML->new();
    my $doc    = $parser->parse_file($filename);

    add_type_to_hash $_ foreach $doc->findnodes('/data-definition/enum-type');
    add_type_to_hash $_ foreach $doc->findnodes('/data-definition/bitfield-type');
    add_type_to_hash $_ foreach $doc->findnodes('/data-definition/struct-type');
    add_type_to_hash $_ foreach $doc->findnodes('/data-definition/class-type');
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

        # Emit the actual type definition
        my @code = with_emit {
            with_anon {
                $type_handlers{$type->nodeName}->($type);
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
    open FH, ">$output_dir/static.inc";
    print FH "/* THIS FILE WAS GENERATED. DO NOT EDIT. */";
    for my $name (sort { $a cmp $b } keys %static_includes) {
        print FH "#include \"$name.h\"";
    }
    print FH "namespace $main_namespace {";
    print FH @static_lines;
    print FH '}';
    close FH;
}
