package StructType;

use utf8;
use strict;
use warnings;

BEGIN {
    use Exporter  ();
    our $VERSION = 1.00;
    our @ISA     = qw(Exporter);
    our @EXPORT  = qw(
        &render_struct_type
    );
    our %EXPORT_TAGS = ( ); # eg: TAG => [ qw!name1 name2! ],
    our @EXPORT_OK   = qw( );
}

END { }

use XML::LibXML;

use Common;
use StructFields;

# MISC

sub translate_lookup($) {
    my ($str) = @_;
    return undef unless $str && $str =~ /^\$global((\.[_a-zA-Z0-9]+)+)$/;
    my @fields = split /\./, substr($1,1);
    my $expr = "df::global::".shift(@fields);
    for my $fn (@fields) {
        $expr = "_toref($expr).$fn";
    }
    return $expr;
}

sub emit_find_instance {
    my ($tag) = @_;

    my $instance_vector = translate_lookup $tag->getAttribute('instance-vector');
    if ($instance_vector) {
        emit "static std::vector<$typename*> &get_vector();";
        emit "static $typename *find(int id);";

        with_emit_static {
            emit_block {
                emit "return ", $instance_vector, ";";
            } "std::vector<$typename*>& ${typename}::get_vector() ";

            emit_block {
                emit "std::vector<$typename*> &vec_ = get_vector();";

                if (my $id = $tag->getAttribute('key-field')) {
                    emit "return binsearch_in_vector(vec_, &${typename}::$id, id_);";
                } else {
                    emit "return (id_ >= 0 && id_ < vec_.size()) ? vec_[id_] : NULL;";
                }
            } "$typename *${typename}::find(int id_) ";
        };
    }
}

sub render_virtual_methods {
    my ($tag) = @_;

    # Collect all parent classes
    my @parents = ( $tag );
    for (;;) {
        my $inherits = $parents[0]->getAttribute('inherits-from') or last;
        my $parent = $types{$inherits} || die "Unknown parent: $inherits\n";
        unshift @parents, $parent;
    }

    # Build the vtable array
    my %name_index;
    my @vtable;
    my @starts;
    my $dtor_id = '~destructor';

    for my $type (@parents) {
        push @starts, scalar(@vtable);
        for my $method ($type->findnodes('virtual-methods/vmethod')) {
            my $is_destructor = is_attr_true($method, 'is-destructor');
            my $name = $is_destructor ? $dtor_id : $method->getAttribute('name');
            if ($name) {
                die "Duplicate method: $name in ".$type->getAttribute('type-name')."\n"
                    if exists $name_index{$name};
                $name_index{$name} = scalar(@vtable);
            }
            push @vtable, $method;
        }
    }

    # Ensure there is a destructor to avoid warnings
    my $dtor_idx = $name_index{$dtor_id};
    unless (defined $dtor_idx) {
        for (my $i = 0; $i <= $#vtable; $i++) {
            next if $vtable[$i]->getAttribute('name');
            $name_index{$dtor_id} = $dtor_idx = $i;
            last;
        }
    }
    unless (defined $dtor_idx) {
        push @vtable, undef;
        $dtor_idx = $#vtable;
    }

    # Generate the methods
    my $min_id = $starts[-1];
    my $cur_mode = '';
    for (my $idx = $min_id; $idx <= $#vtable; $idx++) {
        my $method = $vtable[$idx];
        my $is_destructor = 1;
        my $name = $typename;
        my $is_anon = 1;

        if ($method) {
            $is_destructor = is_attr_true($method, 'is-destructor');
            $name = $method->getAttribute('name') unless $is_destructor;
            $is_anon = 0 if $name;
        }

        my $rq_mode = $is_anon ? 'protected' : 'public';            
        unless ($rq_mode eq $cur_mode) {
            $cur_mode = $rq_mode;
            outdent { emit "$cur_mode:"; }
        }

        with_anon {
            $name = ensure_name $name;
            $method->setAttribute('ld:anon-name', $name) if $method && $is_anon;

            my @ret_type = $is_destructor ? () : $method->findnodes('ret-type');
            my @arg_types = $is_destructor ? () : $method->findnodes('ld:field');
            my $ret_type = $ret_type[0] ? get_struct_field_type($ret_type[0], -local => 1) : 'void';
            my @arg_strs = map { scalar get_struct_field_type($_, -local => 1) } @arg_types;

            my $ret_stmt = '';
            unless ($ret_type eq 'void') {
                $ret_stmt = ' return '.($ret_type =~ /\*$/ ? '0' : "$ret_type()").'; ';
            }

            emit 'virtual ', ($is_destructor?'~':$ret_type.' '), $name,
                 '(', join(', ', @arg_strs), ') {', $ret_stmt, '}; //', $idx;
        } "anon_vmethod_$idx";
    }
}

sub render_struct_type {
    my ($tag) = @_;

    my $tag_name = $tag->getAttribute('ld:meta');
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
        emit_struct_fields($tag, $typename, -class => $is_class, -inherits => $inherits);
        emit_find_instance($tag);
        
        if ($has_methods) {
            if ($is_class) {
                emit "static class_virtual_identity<$typename> _identity;";
                with_emit_static {
                    emit "class_virtual_identity<$typename> ${typename}::_identity(",
                         "\"$typename\",",
                         ($original_name ? "\"$original_name\"" : 'NULL'), ',',
                         ($inherits ? "&${inherits}::_identity" : 'NULL'),
                         ");";
                };
            }

            if ($is_class) {
                render_virtual_methods $tag;
            } else {
                emit "~",$typename,"() {}";
            }
        }
    } $tag, "$typename$ispec", -export => 1;
}

1;
