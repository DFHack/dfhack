package StructFields;

use utf8;
use strict;
use warnings;

BEGIN {
    use Exporter  ();
    our $VERSION = 1.00;
    our @ISA     = qw(Exporter);
    our @EXPORT  = qw(
        *in_struct_body &with_struct_block
        &get_struct_fields &get_struct_field_type
        &emit_struct_fields
    );
    our %EXPORT_TAGS = ( ); # eg: TAG => [ qw!name1 name2! ],
    our @EXPORT_OK   = qw( );
}

END { }

use XML::LibXML;

use Common;
use Enum;
use Bitfield;

# MISC

our $in_struct_body = 0;

sub with_struct_block(&$;$%) {
    my ($blk, $tag, $name, %flags) = @_;
    
    my $kwd = (is_attr_true($tag,'is-union') ? "union" : "struct");
    my $exp = $flags{-export} ? $export_prefix : '';
    my $prefix = $kwd.' '.$exp.($name ? $name.' ' : '');
    
    emit_block {
        local $_;
        local $in_struct_body = 1;
        if ($flags{-no_anon}) {
            $blk->();
        } else {
            &with_anon($blk);
        }
    } $prefix, ";";
}

# FIELD TYPE

sub get_container_item_type($;%) {
    my ($tag, %flags) = @_;
    my @items = $tag->findnodes('ld:item');
    if (@items) {
        return get_struct_field_type($items[0], -local => 1, %flags);
    } elsif ($flags{-void}) {
        return $flags{-void};
    } else {
        die "Container without element: $tag\n";
    }
}

my %atable = ( 1 => 'char', 2 => 'short', 4 => 'int' );

my %custom_primitive_handlers = (
    'stl-string' => sub { return "std::string"; },
);

my %custom_container_handlers = (
    'stl-vector' => sub {
        my $item = get_container_item_type($_, -void => 'void*');
        $item = 'char' if $item eq 'bool';
        return "std::vector<$item>";
    },
    'stl-bit-vector' => sub {
        return "std::vector<bool>";
    },
    'df-flagarray' => sub {
        my $type = decode_type_name_ref($_, -attr_name => 'index-enum', -force_type => 'enum-type') || 'int';
        return "BitArray<$type>";
    },
);

sub emit_typedef($$) {
    # Convert a prefix/postfix pair into a single name
    my ($pre, $post) = @_;
    my $name = ensure_name undef;
    emit 'typedef ', $pre, ' ', $name, $post, ';';
    return $name;
}

sub get_struct_fields($) {
    return $_[0]->findnodes('ld:field');
}

sub get_struct_field_type($;%) {
    # Dispatch on the tag name, and retrieve the type prefix & suffix
    my ($tag, %flags) = @_;
    my $meta = $tag->getAttribute('ld:meta');
    my $subtype = $tag->getAttribute('ld:subtype');
    my $prefix;
    my $suffix = '';

    if ($prefix = $tag->getAttribute('ld:typedef-name')) {
        unless ($flags{-local}) {
            my @names = ( $main_namespace );
            for my $parent ($tag->findnodes('ancestor::*')) {
                if ($parent->nodeName eq 'ld:global-type') {
                    push @names, $parent->getAttribute('type-name');
                } elsif (my $n = $parent->getAttribute('ld:typedef-name')) {
                    push @names, $n;
                }
            }
            $prefix = join('::',@names,$prefix);
        }
    } elsif ($meta eq 'number') {
        $prefix = primitive_type_name($subtype);
    } elsif ($meta eq 'bytes') {
        if ($flags{-local} && !$flags{-weak}) {
            if ($subtype eq 'static-string') {
                my $count = $tag->getAttribute('size') || 0;
                $prefix = "char";
                $suffix = "[$count]";
            } elsif ($subtype eq 'padding') {
                my $count = $tag->getAttribute('size') || 0;
                my $alignment = $tag->getAttribute('alignment') || 1;
                $prefix = $atable{$alignment} or die "Invalid alignment: $alignment\n";
                ($count % $alignment) == 0 or die "Invalid size & alignment: $count $alignment\n";
                $suffix = "[".($count/$alignment)."]";
            } else {
                die "Invalid bytes subtype: $subtype\n";
            }
        } else {
            $prefix = primitive_type_name($subtype);
        }
    } elsif ($meta eq 'global') {
        my $tname = $tag->getAttribute('type-name');
        register_ref $tname, !$flags{-weak};
        $prefix = $main_namespace.'::'.$tname;
    } elsif ($meta eq 'compound') {
        die "Unnamed compound in global mode: ".$tag->toString."\n" unless $flags{-local};

        $prefix = ensure_name undef;
        $tag->setAttribute('ld:typedef-name', $prefix) if $in_struct_body;

        $subtype ||= 'compound';
        if ($subtype eq 'enum') {
            with_anon {
                render_enum_core($prefix,$tag);
            };
        } elsif ($subtype eq 'bitfield') {
            with_anon {
                render_bitfield_core($prefix,$tag);
            };
        } else {
            with_struct_block {
                emit_struct_fields($tag, $prefix);
            } $tag, $prefix;
        }
    } elsif ($meta eq 'pointer') {
        $prefix = get_container_item_type($tag, -weak => 1, -void => 'void')."*";
    } elsif ($meta eq 'static-array') {
        ($prefix, $suffix) = get_container_item_type($tag);
        my $count = $tag->getAttribute('count') || 0;
        $suffix = "[$count]".$suffix;
    } elsif ($meta eq 'primitive') {
        local $_ = $tag;
        my $handler = $custom_primitive_handlers{$subtype} or die "Invalid primitive: $subtype\n";
        $prefix = $handler->($tag, %flags);
    } elsif ($meta eq 'container') {
        local $_ = $tag;
        my $handler = $custom_container_handlers{$subtype} or die "Invalid container: $subtype\n";
        $prefix = $handler->($tag, %flags);
    } elsif (!$flags{-local} && $tag->nodeName eq 'ld:global-type') {
        my $tname = $tag->getAttribute('type-name');
        $prefix = $main_namespace.'::'.$tname;
    } else {
        die "Invalid field meta type: $meta\n";
    }

    if ($subtype && $flags{-local} && $subtype eq 'enum') {
        my $base = get_primitive_base($tag, 'int32_t');
        $prefix = "enum_field<$prefix,$base>";
    }

    return ($prefix,$suffix) if wantarray;
    if ($suffix) {
        $prefix = emit_typedef($prefix, $suffix);
        $tag->setAttribute('ld:typedef-name', $prefix) if $flags{-local} && $in_struct_body;
    }
    return $prefix;
}

sub render_struct_field($) {
    my ($tag) = @_;

    # Special case: anonymous compounds.
    if (is_attr_true($tag, 'ld:anon-compound'))
    {
        check_bad_attrs($tag);
        with_struct_block {
            render_struct_field($_) for get_struct_fields($tag);
        } $tag, undef, -no_anon => 1;
        return;
    }

    # Otherwise, create the name if necessary, and render
    my $field_name = $tag->getAttribute('name');
    my $name = ensure_name $field_name;
    $tag->setAttribute('ld:anon-name', $name) unless $field_name;
    with_anon {
        my ($prefix, $postfix) = get_struct_field_type($tag, -local => 1);
        emit $prefix, ' ', $name, $postfix, ';';
    } "T_$name";
}

our @simple_inits;
our $in_union = 0;

sub render_field_init($$) {
    my ($field, $prefix) = @_;
    local $_;

    my $meta = $field->getAttribute('ld:meta');
    my $subtype = $field->getAttribute('ld:subtype');
    my $name = $field->getAttribute('name') || $field->getAttribute('ld:anon-name');
    my $fname = ($prefix && $name ? $prefix.'.'.$name : ($name||$prefix));

    my $is_struct = $meta eq 'compound' && !$subtype;
    my $is_union = ($is_struct && is_attr_true($field, 'is-union'));
    local $in_union = $in_union || $is_union;

    if (is_attr_true($field, 'ld:anon-compound') || ($in_union && $is_struct))
    {
        my @fields = $is_union ? $field->findnodes('ld:field[1]') : get_struct_fields($field);
        &render_field_init($_, $fname) for @fields;
        return;
    }

    return unless ($name || $prefix =~ /\]$/);

    my $val = $field->getAttribute('init-value');    
    my $assign = 0;

    if ($meta eq 'number' || $meta eq 'pointer') {
        $assign = 1;
        my $signed_ref =
            !is_attr_true($field,'ld:unsigned') &&
            ($field->getAttribute('ref-target') || $field->getAttribute('refers-to'));
        $val ||= ($signed_ref ? '-1' : 0);
    } elsif ($meta eq 'bytes') {
        emit "memset($fname, 0, sizeof($fname));";
    } elsif ($meta eq 'global' || $meta eq 'compound') {
        return unless $subtype;

        if ($subtype eq 'bitfield' && $val) {
            emit $fname, '.whole = ', $val;
        } elsif ($subtype eq 'enum') {
            $assign = 1;
            if ($meta eq 'global') {
                my $tname = $field->getAttribute('type-name');
                $val = ($val ? $main_namespace.'::enums::'.$tname.'::'.$val : "ENUM_FIRST_ITEM($tname)");
            } else {
                $val ||= $field->findvalue('enum-item[1]/@name');
            }
        }
    } elsif ($meta eq 'static-array') {
        my $idx = ensure_name undef;
        my $count = $field->getAttribute('count')||0;
        emit_block {
            my $pfix = $fname."[$idx]";
            render_field_init($_, $pfix) for $field->findnodes('ld:item');
        } "for (int $idx = 0; $idx < $count; $idx++) ", "", -auto => 1;
    }

    if ($assign) {
        if ($prefix || $in_union) {
            emit "$fname = $val;";
        } else {
            push @simple_inits, "$name($val)";
        }
    }
}

sub emit_struct_fields($$;%) {
    my ($tag, $name, %flags) = @_;

    local $_;
    my @fields = get_struct_fields($tag);
    &render_struct_field($_) for @fields;

    return if $tag->findnodes("ancestor-or-self::ld:field[\@is-union='true']");

    local $in_struct_body = 0;

    my $want_ctor = 0;
    my $ctor_args = '';
    my $ctor_arg_init = '';

    with_emit_static {
        local @simple_inits;
        my @ctor_lines = with_emit {
            if ($flags{-class}) {
                $ctor_args = "virtual_identity *_id";
                $ctor_arg_init = " = &".$name."::_identity";
                push @simple_inits, "$flags{-inherits}(_id)" if $flags{-inherits};
                emit "_identity.adjust_vtable(this, _id);";
            }
            render_field_init($_, '') for @fields;
        };
        if (@simple_inits || @ctor_lines) {
            $want_ctor = 1;
            my $full_name = get_struct_field_type($tag);
            emit $full_name,'::',$name,"($ctor_args)";
            emit "  :  ", join(', ', @simple_inits) if @simple_inits;
            emit_block {
                emit $_ for @ctor_lines;
            };
        }
    } 'ctors';

    if ($want_ctor) {
        emit "$name($ctor_args$ctor_arg_init);";
    }
}

1;
