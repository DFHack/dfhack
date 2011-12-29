package Enum;

use utf8;
use strict;
use warnings;

BEGIN {
    use Exporter  ();
    our $VERSION = 1.00;
    our @ISA     = qw(Exporter);
    our @EXPORT  = qw( &render_enum_core &render_enum_type );
    our %EXPORT_TAGS = ( ); # eg: TAG => [ qw!name1 name2! ],
    our @EXPORT_OK   = qw( );
}

END { }

use XML::LibXML;

use Common;

sub render_enum_core($$) {
    my ($name,$tag) = @_;

    my $base = 0;

    emit_block {
        my @items = $tag->findnodes('child::enum-item');
        my $idx = 0;

        for my $item (@items) {
            my $name = ensure_name $item->getAttribute('name');
            my $value = $item->getAttribute('value');

            $base = ($idx == 0) ? $value : undef if defined $value;
            $idx++;

            emit $name, (defined($value) ? ' = '.$value : ''), ',';
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
    my @aprefix = ('');

    for my $attr ($tag->findnodes('child::enum-attr')) {
        my $name = $attr->getAttribute('name') or die "Unnamed enum-attr.\n";
        my $type = decode_type_name_ref $attr;
        my $def = $attr->getAttribute('default-value');

        my $base_tname = ($type && $type =~ /::(.*)$/ ? $1 : '');
        $type = $base_tname if $base_tname eq $typename;

        die "Duplicate attribute $name.\n" if exists $aidx{$name};

        check_name $name;
        $aidx{$name} = scalar @anames;
        push @anames, $name;
        push @atnames, $type;

        if ($type) {
            push @atypes, $type;
            push @aprefix, ($base_tname ? $base_tname."::" : '');
            push @avals, (defined $def ? $aprefix[-1].$def : "($type)0");
        } else {
            push @atypes, 'const char*';
            push @avals, (defined $def ? "\"$def\"" : 'NULL');
            push @aprefix, '';
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
                    for my $item ($tag->findnodes('child::enum-item')) {
                        my $tag = $item->nodeName;
                        
                        # Assemble item-specific attr values
                        my @evals = @avals;
                        my $name = $item->getAttribute('name');
                        $evals[0] = "\"$name\"" if $name;

                        for my $attr ($item->findnodes('child::item-attr')) {
                            my $name = $attr->getAttribute('name') or die "Unnamed item-attr.\n";
                            my $value = $attr->getAttribute('value') or die "No-value item-attr.\n";
                            my $idx = $aidx{$name} or die "Unknown item-attr: $name\n";

                            if ($atnames[$idx]) {
                                $evals[$idx] = $aprefix[$idx].$value;
                            } else {
                                $evals[$idx] = "\"$value\"";
                            }
                        }

                        emit "{ ",join(', ',@evals)," },";
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
    } 'enums';
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

1;
