# definition of classes used by ruby-autogen
module DFHack
    module MemHack
        INSPECT_SIZE_LIMIT=16384
        class MemStruct
            attr_accessor :_memaddr
            def _at(addr) ; d = dup ; d._memaddr = addr ; d ; end
            def _get ; self ; end
            def _cpp_init ; end
            def _cpp_delete ; end
        end

        class Compound < MemStruct
            class << self
                attr_accessor :_fields, :_rtti_classname, :_sizeof
                def field(name, offset)
                    struct = yield
                    return if not struct
                    @_fields ||= []
                    @_fields << [name, offset, struct]
                    define_method(name) { struct._at(@_memaddr+offset)._get }
                    define_method("#{name}=") { |v| struct._at(@_memaddr+offset)._set(v) }
                end
                def _fields_ancestors
                    if superclass.respond_to?(:_fields_ancestors)
                        superclass._fields_ancestors + _fields.to_a
                    else
                        _fields.to_a
                    end
                end

                def number(bits, signed, initvalue=nil, enum=nil)
                    Number.new(bits, signed, initvalue, enum)
                end
                def float
                    Float.new
                end
                def double
                    Double.new
                end
                def bit(shift, enum=nil)
                    BitField.new(shift, 1, enum)
                end
                def bits(shift, len, enum=nil)
                    BitField.new(shift, len, enum)
                end
                def pointer
                    Pointer.new((yield if block_given?))
                end
                def pointer_ary(tglen)
                    PointerAry.new(tglen, yield)
                end
                def static_array(len, tglen, indexenum=nil)
                    StaticArray.new(tglen, len, indexenum, yield)
                end
                def static_string(len)
                    StaticString.new(len)
                end

                def stl_vector(tglen=nil)
                    tg = yield if tglen
                    case tglen
                    when 1; StlVector8.new(tg)
                    when 2; StlVector16.new(tg)
                    else StlVector32.new(tg)
                    end
                end
                def stl_string
                    StlString.new
                end
                def stl_bit_vector
                    StlBitVector.new
                end
                def stl_deque(tglen)
                    StlDeque.new(tglen, yield)
                end

                def df_flagarray(indexenum=nil)
                    DfFlagarray.new(indexenum)
                end
                def df_static_flagarray(len, indexenum=nil)
                    DfStaticFlagarray.new(len, indexenum)
                end
                def df_array(tglen)
                    DfArray.new(tglen, yield)
                end
                def df_linked_list
                    DfLinkedList.new(yield)
                end

                def global(glob)
                    Global.new(glob)
                end
                def compound(name=nil, &b)
                    m = Class.new(Compound)
                    DFHack.const_set(name, m) if name
                    m.class_eval(&b)
                    m.new
                end
                def rtti_classname(n)
                    DFHack.rtti_register(n, self)
                    @_rtti_classname = n
                end
                def sizeof(n)
                    @_sizeof = n
                end

                # allocate a new c++ object, return its ruby wrapper
                def cpp_new(init=nil)
                    ptr = DFHack.malloc(_sizeof)
                    if _rtti_classname and vt = DFHack.rtti_getvtable(_rtti_classname)
                        DFHack.memory_write_int32(ptr, vt)
                        # TODO call constructor
                    end
                    o = new._at(ptr)
                    o._cpp_init
                    o._set(init) if init
                    o
                end
            end
            def _cpp_init
                _fields_ancestors.each { |n, o, s| s._at(@_memaddr+o)._cpp_init }
            end
            def _cpp_delete
                # cannot call delete on compound members (would call free on member address)
                #_fields_ancestors.each { |n, o, s| s._at(@_memaddr+o)._cpp_delete }
                DFHack.free(@_memaddr)
                @_memaddr = nil     # turn future segfaults in harmless ruby exceptions
            end
            def _set(h)
                case h
                when Hash; h.each { |k, v| send("#{k}=", v) }
                when Array; names = _field_names ; raise 'bad size' if names.length != h.length ; names.zip(h).each { |n, a| send("#{n}=", a) }
                else _field_names.each { |n| send("#{n}=", h.send(n)) }
                end
            end
            def _fields ; self.class._fields.to_a ; end
            def _fields_ancestors ; self.class._fields_ancestors.to_a ; end
            def _field_names ; _fields_ancestors.map { |n, o, s| n } ; end
            def _rtti_classname ; self.class._rtti_classname ; end
            def _raw_rtti_classname ; df.get_rtti_classname(df.get_vtable_ptr(@_memaddr)) if self.class._rtti_classname ; end
            def _sizeof ; self.class._sizeof ; end
            def ==(o) ; o.kind_of?(Compound) and o._memaddr == _memaddr ; end

            @@inspecting = {} # avoid infinite recursion on mutually-referenced objects
            def inspect
                cn = self.class.name.sub(/^DFHack::/, '')
                out = "#<#{cn}"
                return out << ' ...>' if @@inspecting[_memaddr]
                @@inspecting[_memaddr] = true
                _fields_ancestors.each { |n, o, s|
                    out << ' ' if out.length != 0 and out[-1, 1] != ' '
                    if out.length > INSPECT_SIZE_LIMIT
                        out << '...'
                        break
                    end
                    out << inspect_field(n, o, s)
                }
                out.chomp!(' ')
                @@inspecting.delete _memaddr
                out << '>'
            end
            def inspect_field(n, o, s)
                if s.kind_of?(BitField) and s._len == 1 and not s._enum
                    send(n) ? n.to_s : ''
                elsif s.kind_of?(Pointer)
                    "#{n}=#{s._at(@_memaddr+o).inspect}"
                elsif n == :_whole
                    "_whole=0x#{_whole.to_s(16)}"
                else
                    v = send(n).inspect
                    "#{n}=#{v}"
                end
            rescue
                "#{n}=ERR(#{$!})"
            end
        end

        class Enum
            # number -> symbol
            def self.enum
                # ruby weirdness, needed to make the constants 'virtual'
                @enum ||= const_get(:ENUM)
            end
            # symbol -> number
            def self.nume
                @nume ||= const_get(:NUME)
            end

            def self.int(i, allow_bad_sym=false)
                raise ArgumentError, "invalid enum member #{i} of #{self}" if i.kind_of?(::Symbol) and not allow_bad_sym and not nume.has_key?(i)
                nume[i] || i
            end
            def self.sym(i)
                enum[i] || i
            end
        end

        class Number < MemStruct
            attr_accessor :_bits, :_signed, :_initvalue, :_enum
            def initialize(bits, signed, initvalue, enum)
                @_bits = bits
                @_signed = signed
                @_initvalue = initvalue
                @_enum = enum
            end

            def _get
                v = case @_bits
                    when 32; DFHack.memory_read_int32(@_memaddr)
                    when 16; DFHack.memory_read_int16(@_memaddr)
                    when 8;  DFHack.memory_read_int8( @_memaddr)
                    when 64;(DFHack.memory_read_int32(@_memaddr) & 0xffffffff) + (DFHack.memory_read_int32(@_memaddr+4) << 32)
                    end
                v &= (1 << @_bits) - 1 if not @_signed
                v = @_enum.sym(v) if @_enum
                v
            end

            def _set(v)
                v = @_enum.int(v) if @_enum
                case @_bits
                when 32; DFHack.memory_write_int32(@_memaddr, v)
                when 16; DFHack.memory_write_int16(@_memaddr, v)
                when 8;  DFHack.memory_write_int8( @_memaddr, v)
                when 64; DFHack.memory_write_int32(@_memaddr, v & 0xffffffff) ; DFHack.memory_write_int32(@memaddr+4, v>>32)
                end
            end

            def _cpp_init
                _set(@_initvalue) if @_initvalue
            end
        end
        class Float < MemStruct
            def _get
                DFHack.memory_read_float(@_memaddr)
            end

            def _set(v)
                DFHack.memory_write_float(@_memaddr, v)
            end

            def _cpp_init
                _set(0.0)
            end
        end
        class Double < MemStruct
            def _get
                DFHack.memory_read_double(@_memaddr)
            end

            def _set(v)
                DFHack.memory_write_double(@_memaddr, v)
            end

            def _cpp_init
                _set(0.0)
            end
        end
        class BitField < MemStruct
            attr_accessor :_shift, :_len, :_enum
            def initialize(shift, len, enum=nil)
                @_shift = shift
                @_len = len
                @_enum = enum
            end
            def _mask
                (1 << @_len) - 1
            end

            def _get
                v = DFHack.memory_read_int32(@_memaddr) >> @_shift
                if @_len == 1 and not @_enum
                    ((v & 1) == 0) ? false : true
                else
                    v &= _mask
                    v = @_enum.sym(v) if @_enum
                    v
                end
            end

            def _set(v)
                if @_len == 1 and (not @_enum or v == false or v == true)
                    # allow 'bit = 0'
                    v = (v && v != 0 ? 1 : 0)
                end
                v = @_enum.int(v) if @_enum
                v = (v & _mask) << @_shift

                ori = DFHack.memory_read_int32(@_memaddr) & 0xffffffff
                DFHack.memory_write_int32(@_memaddr, ori - (ori & ((-1 & _mask) << @_shift)) + v)
            end
        end

        class Pointer < MemStruct
            attr_accessor :_tg
            def initialize(tg)
                @_tg = tg
            end

            def _getp
                DFHack.memory_read_int32(@_memaddr) & 0xffffffff
            end

            def _setp(v)
               DFHack.memory_write_int32(@_memaddr, v)
            end

            def _get
                addr = _getp
                return if addr == 0
                return addr if not @_tg
                @_tg._at(addr)._get
            end

            # XXX shaky...
            def _set(v)
                case v
                when Pointer;   DFHack.memory_write_int32(@_memaddr, v._getp)
                when MemStruct; DFHack.memory_write_int32(@_memaddr, v._memaddr)
                when Integer
                    if @_tg and @_tg.kind_of?(MemHack::Number)
                        if _getp == 0
                            _setp(DFHack.malloc(@_tg._bits/8))
                        end
                        @_tg._at(_getp)._set(v)
                    else
                        DFHack.memory_write_int32(@_memaddr, v)
                    end
                when nil;       DFHack.memory_write_int32(@_memaddr, 0)
                else @_tg._at(_getp)._set(v)
                end
            end

            def inspect
                ptr = _getp
                if ptr == 0
                    'NULL'
                else
                    cn = ''
                    cn = @_tg.class.name.sub(/^DFHack::/, '').sub(/^MemHack::/, '') if @_tg
                    cn = @_tg._glob if cn == 'Global'
                    "#<Pointer #{cn} #{'0x%X' % _getp}>"
                end
            end
        end
        class PointerAry < MemStruct
            attr_accessor :_tglen, :_tg
            def initialize(tglen, tg)
                @_tglen = tglen
                @_tg = tg
            end

            def _getp(i=0)
                delta = (i != 0 ? i*@_tglen : 0)
                (DFHack.memory_read_int32(@_memaddr) & 0xffffffff) + delta
            end

            def _get
                addr = _getp
                return if addr == 0
                self
            end

            def _set(v)
                case v
                when Pointer;   DFHack.memory_write_int32(@_memaddr, v._getp)
                when MemStruct; DFHack.memory_write_int32(@_memaddr, v._memaddr)
                when Integer;   DFHack.memory_write_int32(@_memaddr, v)
                when nil;       DFHack.memory_write_int32(@_memaddr, 0)
                else raise "cannot PointerAry._set(#{v.inspect})"
                end
            end

            def [](i)
                addr = _getp(i)
                return if addr == 0
                @_tg._at(addr)._get
            end
            def []=(i, v)
                addr = _getp(i)
                raise 'null pointer' if addr == 0
                @_tg._at(addr)._set(v)
            end

            def inspect ; ptr = _getp ; (ptr == 0) ? 'NULL' : "#<PointerAry #{'0x%X' % ptr}>" ; end
        end
        module Enumerable
            include ::Enumerable
            attr_accessor :_indexenum
            def each ; (0...length).each { |i| yield self[i] } ; end
            def inspect
                out = '['
                each_with_index { |e, idx|
                    out << ', ' if out.length > 1
                    if out.length > INSPECT_SIZE_LIMIT
                        out << '...'
                        break
                    end
                    out << "#{_indexenum.sym(idx)}=" if _indexenum
                    out << e.inspect
                }
                out << ']'
            end
            def empty? ; length == 0 ; end
            def flatten ; map { |e| e.respond_to?(:flatten) ? e.flatten : e }.flatten ; end
            def index(e=nil, &b) ; (0...length).find { |i| b ? b[self[i]] : self[i] == e } ; end
            def map! ; (0...length).each { |i| self[i] = yield(self[i]) } ; end
            def first ; self[0] ; end
            def last ; self[length-1] ; end
        end
        class StaticArray < MemStruct
            attr_accessor :_tglen, :_length, :_indexenum, :_tg
            def initialize(tglen, length, indexenum, tg)
                @_tglen = tglen
                @_length = length
                @_indexenum = indexenum
                @_tg = tg
            end
            def _set(a)
                a.each_with_index { |v, i| self[i] = v }
            end
            def _cpp_init
                _length.times { |i| _tgat(i)._cpp_init }
            end
            def _cpp_delete
                _length.times { |i| _tgat(i)._cpp_delete }
            end
            alias length _length
            alias size _length
            def _tgat(i)
                @_tg._at(@_memaddr + i*@_tglen) if i >= 0 and i < @_length
            end
            def [](i)
                i = _indexenum.int(i) if _indexenum
                i += @_length if i < 0
                if t = _tgat(i)
                    t._get
                end
            end
            def []=(i, v)
                i = _indexenum.int(i) if _indexenum
                i += @_length if i < 0
                if t = _tgat(i)
                    t._set(v)
                else
                    raise 'index out of bounds'
                end
            end

            include Enumerable
        end
        class StaticString < MemStruct
            attr_accessor :_length
            def initialize(length)
                @_length = length
            end
            def length
                if @_length == -1
                    maxlen = 4096 - (@_memaddr & 0xfff)
                    maxlen += 4096 until len = DFHack.memory_read(@_memaddr, maxlen).index(?\0)
                    len
                else
                    @_length
                end
            end
            def _get
                DFHack.memory_read(@_memaddr, length)
            end
            def _set(v)
                DFHack.memory_write(@_memaddr, v[0, length])
            end
        end

        class StlVector32 < MemStruct
            attr_accessor :_tg
            def initialize(tg)
                @_tg = tg || Number.new(32, false, 0, nil)
            end

            def length
                DFHack.memory_vector32_length(@_memaddr)
            end
            def size ; length ; end # alias wouldnt work for subclasses
            def valueptr_at(idx)
                DFHack.memory_vector32_ptrat(@_memaddr, idx)
            end
            def insert_at(idx, val)
                DFHack.memory_vector32_insertat(@_memaddr, idx, val)
            end
            def delete_at(idx)
                DFHack.memory_vector32_deleteat(@_memaddr, idx)
            end

            def _set(v)
                delete_at(length-1) while length > v.length # match lengthes
                v.each_with_index { |e, i| self[i] = e }    # patch entries
            end

            def self._cpp_new
                new._at DFHack.memory_vector_new
            end
            def _cpp_delete
                DFHack.memory_vector_delete(@_memaddr)
            end
            def _cpp_init
                DFHack.memory_vector_init(@_memaddr)
            end

            def clear
                delete_at(length-1) while length > 0
            end
            def [](idx)
                idx += length if idx < 0
                @_tg._at(valueptr_at(idx))._get if idx >= 0 and idx < length
            end
            def []=(idx, v)
                idx += length if idx < 0
                if idx >= length
                    insert_at(length, 0) while idx >= length
                elsif idx < 0
                    raise 'index out of bounds'
                end
                @_tg._at(valueptr_at(idx))._set(v)
            end
            def push(v)
                self[length] = v
                self
            end
            def <<(v) ; push(v) ; end
            def pop
                l = length
                if l > 0
                    v = self[l-1]
                    delete_at(l-1)
                end
                v
            end

            include Enumerable
            # do a binary search in an ordered vector for a specific target attribute
            # ex: world.history.figures.binsearch(unit.hist_figure_id)
            def binsearch(target, field=:id)
                o_start = 0
                o_end = length - 1
                while o_end >= o_start
                    o_half = o_start + (o_end-o_start)/2
                    obj = self[o_half]
                    oval = obj.send(field)
                    if oval == target
                        return obj
                    elsif oval < target
                        o_start = o_half+1
                    else
                        o_end = o_half-1
                    end
                end
            end
        end
        class StlVector16 < StlVector32
            def length
                DFHack.memory_vector16_length(@_memaddr)
            end
            def valueptr_at(idx)
                DFHack.memory_vector16_ptrat(@_memaddr, idx)
            end
            def insert_at(idx, val)
                DFHack.memory_vector16_insertat(@_memaddr, idx, val)
            end
            def delete_at(idx)
                DFHack.memory_vector16_deleteat(@_memaddr, idx)
            end
        end
        class StlVector8 < StlVector32
            def length
                DFHack.memory_vector8_length(@_memaddr)
            end
            def valueptr_at(idx)
                DFHack.memory_vector8_ptrat(@_memaddr, idx)
            end
            def insert_at(idx, val)
                DFHack.memory_vector8_insertat(@_memaddr, idx, val)
            end
            def delete_at(idx)
                DFHack.memory_vector8_deleteat(@_memaddr, idx)
            end
        end
        class StlBitVector < StlVector32
            def initialize ; end
            def length
                DFHack.memory_vectorbool_length(@_memaddr)
            end
            def insert_at(idx, val)
                DFHack.memory_vectorbool_insertat(@_memaddr, idx, val)
            end
            def delete_at(idx)
                DFHack.memory_vectorbool_deleteat(@_memaddr, idx)
            end
            def [](idx)
                idx += length if idx < 0
                DFHack.memory_vectorbool_at(@_memaddr, idx) if idx >= 0 and idx < length
            end
            def []=(idx, v)
                idx += length if idx < 0
                if idx >= length
                    insert_at(idx, v)
                elsif idx < 0
                    raise 'index out of bounds'
                else
                    DFHack.memory_vectorbool_setat(@_memaddr, idx, v)
                end
            end
            def self._cpp_new
                new._at DFHack.memory_vectorbool_new
            end
            def _cpp_delete
                DFHack.memory_vectorbool_delete(@_memaddr)
            end
        end
        class StlString < MemStruct
            def _get
                DFHack.memory_read_stlstring(@_memaddr)
            end

            def _set(v)
                DFHack.memory_write_stlstring(@_memaddr, v)
            end

            def self._cpp_new
                new._at DFHack.memory_stlstring_new
            end
            def _cpp_delete
                DFHack.memory_stlstring_delete(@_memaddr)
            end
            def _cpp_init
                DFHack.memory_stlstring_init(@_memaddr)
            end
        end
        class StlDeque < MemStruct
            attr_accessor :_tglen, :_tg
            def initialize(tglen, tg)
                @_tglen = tglen
                @_tg = tg
            end
            # XXX DF uses stl::deque<some_struct>, so to have a C binding we'd need to single-case every
            # possible struct size, like for StlVector. Just ignore it for now, deques are rare enough.
            def inspect ; "#<StlDeque>" ; end
        end

        class DfFlagarray < MemStruct
            attr_accessor :_indexenum
            def initialize(indexenum)
                @_indexenum = indexenum
            end
            def length
                DFHack.memory_bitarray_length(@_memaddr)
            end
            # TODO _cpp_init, _cpp_delete
            def size ; length ; end
            def resize(len)
                DFHack.memory_bitarray_resize(@_memaddr, len)
            end
            def [](idx)
                idx = _indexenum.int(idx) if _indexenum
                idx += length if idx < 0
                DFHack.memory_bitarray_isset(@_memaddr, idx) if idx >= 0 and idx < length
            end
            def []=(idx, v)
                idx = _indexenum.int(idx) if _indexenum
                idx += length if idx < 0
                if idx >= length or idx < 0
                    raise 'index out of bounds'
                else
                    DFHack.memory_bitarray_set(@_memaddr, idx, v)
                end
            end
            def inspect
                out = "#<DfFlagarray"
                each_with_index { |e, idx|
                    out << " #{_indexenum ? _indexenum.sym(idx) : idx}" if e
                }
                out << '>'
            end

            include Enumerable
        end
        class DfStaticFlagarray < MemStruct
            attr_accessor :_indexenum
            def initialize(len, indexenum)
                @len = len*8
                @_indexenum = indexenum
            end
            def length
                @len
            end
            def size ; length ; end
            def [](idx)
                idx = _indexenum.int(idx) if _indexenum
                idx += length if idx < 0
                return if idx < 0 or idx >= length
                byte = DFHack.memory_read_int8(@_memaddr + idx/8)
                (byte & (1 << (idx%8))) > 0
            end
            def []=(idx, v)
                idx = _indexenum.int(idx) if _indexenum
                idx += length if idx < 0
                if idx >= length or idx < 0
                    raise 'index out of bounds'
                else
                    byte = DFHack.memory_read_int8(@_memaddr + idx/8)
                    if (v == nil or v == false or v == 0)
                        byte &= 0xff ^ (1 << (idx%8))
                    else
                        byte |= (1 << (idx%8))
                    end
                    DFHack.memory_write_int8(@_memaddr + idx/8, byte)
                end
            end
            def inspect
                out = "#<DfStaticFlagarray"
                each_with_index { |e, idx|
                    out << " #{_indexenum.sym(idx)}" if e
                }
                out << '>'
            end

            include Enumerable
        end
        class DfArray < Compound
            attr_accessor :_tglen, :_tg
            def initialize(tglen, tg)
                @_tglen = tglen
                @_tg = tg
            end

            field(:_ptr, 0) { number 32, false }
            field(:_length, 4) { number 16, false }

            def length ; _length ; end
            def size ; _length ; end
            # TODO _cpp_init, _cpp_delete
            def _tgat(i)
                @_tg._at(_ptr + i*@_tglen) if i >= 0 and i < _length
            end
            def [](i)
                i += _length if i < 0
                if t = _tgat(i)
                    t._get
                end
            end
            def []=(i, v)
                i += _length if i < 0
                if t = _tgat(i)
                    t._set(v)
                else
                    raise 'index out of bounds'
                end
            end
            def _set(a)
                a.each_with_index { |v, i| self[i] = v }
            end

            include Enumerable
        end
        class DfLinkedList < Compound
            attr_accessor :_tg
            def initialize(tg)
                @_tg = tg
            end

            field(:_ptr, 0) { pointer }
            field(:_prev, 4) { pointer }
            field(:_next, 8) { pointer }

            def item
                # With the current xml structure, currently _tg designate
                # the type of the 'next' and 'prev' fields, not 'item'.
                # List head has item == NULL, so we can safely return nil.

                #addr = _ptr
                #return if addr == 0
                #@_tg._at(addr)._get
            end

            def item=(v)
                #addr = _ptr
                #raise 'null pointer' if not addr
                #@_tg.at(addr)._set(v)
                raise 'null pointer'
            end

            def prev
                addr = _prev
                return if not addr
                @_tg._at(addr)._get
            end

            def next
                addr = _next
                return if not addr
                @_tg._at(addr)._get
            end
            alias next= _next=
            alias prev= _prev=

            include Enumerable
            def each
                o = self
                while o
                    yield o.item if o.item
                    o = o.next
                end
            end
            def inspect ; "#<DfLinkedList #{item.inspect} prev=#{'0x%X' % _prev} next=#{'0x%X' % _next}>" ; end
        end

        class Global < MemStruct
            attr_accessor :_glob
            def initialize(glob)
                @_glob = glob
            end
            def _at(addr)
                g = DFHack.const_get(@_glob)
                g = DFHack.rtti_getclassat(g, addr)
                g.new._at(addr)
            end
            def inspect ; "#<#{@_glob}>" ; end
        end

    end

    class BooleanEnum
        def self.int(v) ; ((v == true) || (v == 1)) ? 1 : 0 ; end
        def self.sym(v) ; (!v || (v == 0)) ? false : true ; end
    end

    class StlString < MemHack::Compound
        field(:str, 0) { stl_string }

        def self.cpp_new(init=nil)
            s = MemHack::StlString._cpp_new
            s._set(init) if init
            new._at(s._memaddr)
        end

        def _cpp_delete
            MemHack::StlString.new._at(@_memaddr+0)._cpp_delete
            @_memaddr = nil
        end
    end

    class StlSet
        attr_accessor :_memaddr, :_enum
        def self.cpp_new(init=nil, enum=nil)
            ret = new DFHack.memory_stlset_new, enum
            init.each { |k| ret.set(k) } if init
            ret
        end

        def initialize(addr, enum=nil)
            addr = nil if addr == 0
            @_memaddr = addr
            @_enum = enum
        end

        def isset(key)
            raise unless @_memaddr
            key = @_enum.int(key) if _enum
            DFHack.memory_stlset_isset(@_memaddr, key)
        end
        alias is_set? isset

        def set(key)
            raise unless @_memaddr
            key = @_enum.int(key) if _enum
            DFHack.memory_stlset_set(@_memaddr, key)
        end

        def delete(key)
            raise unless @_memaddr
            key = @_enum.int(key) if _enum
            DFHack.memory_stlset_deletekey(@_memaddr, key)
        end

        def clear
            raise unless @_memaddr
            DFHack.memory_stlset_clear(@_memaddr)
        end

        def _cpp_delete
            raise unless @_memaddr
            DFHack.memory_stlset_delete(@_memaddr)
            @_memaddr = nil
        end
    end


    # cpp rtti name -> rb class
    @rtti_n2c = {}
    @rtti_c2n = {}

    # cpp rtti name -> vtable ptr
    @rtti_n2v = {}
    @rtti_v2n = {}

    def self.rtti_n2c ; @rtti_n2c ; end
    def self.rtti_c2n ; @rtti_c2n ; end
    def self.rtti_n2v ; @rtti_n2v ; end
    def self.rtti_v2n ; @rtti_v2n ; end

    # register a ruby class with a cpp rtti class name
    def self.rtti_register(cppname, cls)
        @rtti_n2c[cppname] = cls
        @rtti_c2n[cls] = cppname
    end

    # return the ruby class to use for the cpp object at address if rtti info is available
    def self.rtti_getclassat(cls, addr)
        if addr != 0 and @rtti_c2n[cls]
            # rtti info exist for class => cpp object has a vtable
            @rtti_n2c[rtti_readclassname(get_vtable_ptr(addr))] || cls
        else
            cls
        end
    end

    # try to read the rtti classname from an object vtable pointer
    def self.rtti_readclassname(vptr)
        unless n = @rtti_v2n[vptr]
            n = @rtti_v2n[vptr] = get_rtti_classname(vptr).to_sym
            @rtti_n2v[n] = vptr
        end
        n
    end

    # return the vtable pointer from the cpp rtti name
    def self.rtti_getvtable(cppname)
        unless v = @rtti_n2v[cppname]
            v = get_vtable(cppname.to_s)
            @rtti_n2v[cppname] = v
            @rtti_v2n[v] = cppname if v != 0
        end
        v if v != 0
    end

    def self.vmethod_call(obj, voff, a0=0, a1=0, a2=0, a3=0, a4=0, a5=0)
        this = obj._memaddr
        vt = df.get_vtable_ptr(this)
        fptr = df.memory_read_int32(vt + voff) & 0xffffffff
        vmethod_do_call(this, fptr, vmethod_arg(a0), vmethod_arg(a1), vmethod_arg(a2),
                        vmethod_arg(a3), vmethod_arg(a4), vmethod_arg(a5))
    end

    def self.vmethod_arg(arg)
        case arg
        when nil, false; 0
        when true; 1
        when Integer; arg
        #when String; [arg].pack('p').unpack('L')[0] # raw pointer to buffer
        when MemHack::Compound, StlSet; arg._memaddr
        else raise "bad vmethod arg #{arg.class}"
        end
    end
end
