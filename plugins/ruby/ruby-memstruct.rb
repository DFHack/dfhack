module DFHack
module MemHack
class MemStruct
	attr_accessor :_memaddr
	def _at(addr) ; d = dup ; d._memaddr = addr ; d ; end
	def _get ; self ; end
	def _cpp_init ; end
	def inspect ; _get.inspect ; end
end

class Compound < MemStruct
	class << self
		attr_accessor :_fields, :_rtti_classname, :_sizeof
		def field(name, offset)
			struct = yield
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

		def number(bits, signed, initvalue=nil)
			Number.new(bits, signed, initvalue)
		end
		def float
			Float.new
		end
		def bit(shift)
			BitField.new(shift, 1)
		end
		def bits(shift, len)
			BitField.new(shift, len)
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
		def df_array(tglen)
			DfArray.new(tglen, yield)
		end
		def df_linked_list
			DfLinkedList.new(yield)
		end

		def global(glob)
			Global.new(glob)
		end
		def compound(&b)
			m = Class.new(Compound)
			m.instance_eval(&b)
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
		def cpp_new
			ptr = DFHack.malloc(_sizeof)
			if _rtti_classname and vt = DFHack.rtti_getvtable(_rtti_classname)
				DFHack.memory_write_int32(ptr, vt)
				# TODO call constructor
			end
			o = new._at(ptr)
			o._cpp_init
			o
		end
	end
	def _cpp_init
		_fields_ancestors.each { |n, o, s| s._at(@_memaddr+o)._cpp_init }
	end
	def _set(h) ; h.each { |k, v| send("_#{k}=", v) } ; end
	def _fields ; self.class._fields.to_a ; end
	def _fields_ancestors ; self.class._fields_ancestors.to_a ; end
	def _rtti_classname ; self.class._rtti_classname ; end
	def _sizeof ; self.class._sizeof ; end
	def inspect
		cn = self.class.name.sub(/^DFHack::/, '')
		cn << ' @' << ('0x%X' % _memaddr) if cn != ''
		out = "#<#{cn}"
		_fields_ancestors.each { |n, o, s|
			out << ' ' if out.length != 0 and out[-1, 1] != ' '
			if out.length > 1024
				out << '...'
				break
			end
			out << inspect_field(n, o, s)
		}
		out << '>'
	end
	def inspect_field(n, o, s)
		if s.kind_of?(BitField) and s._len == 1
			send(n) ? n.to_s : ''
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

class Number < MemStruct
	attr_accessor :_bits, :_signed, :_initvalue
	def initialize(bits, signed, initvalue)
		@_bits = bits
		@_signed = signed
		@_initvalue = initvalue
	end

	def _get
		v = case @_bits
		when 32; DFHack.memory_read_int32(@_memaddr)
		when 16; DFHack.memory_read_int16(@_memaddr)
		when 8;  DFHack.memory_read_int8( @_memaddr)
		when 64;(DFHack.memory_read_int32(@_memaddr) & 0xffffffff) + (DFHack.memory_read_int32(@_memaddr+4) << 32)
		end
		v &= (1 << @_bits) - 1 if not @_signed
		v
	end

	def _set(v)
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
	# _get/_set defined in ruby.cpp
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
class BitField < MemStruct
	attr_accessor :_shift, :_len
	def initialize(shift, len)
		@_shift = shift
		@_len = len
	end
	def _mask
		(1 << @_len) - 1
	end

	def _get
		v = DFHack.memory_read_int32(@_memaddr) >> @_shift
		if @_len == 1
			((v & 1) == 0) ? false : true
		else
			v & _mask
		end
	end

	def _set(v)
		if @_len == 1
			# allow 'bit = 0'
			v = (v && v != 0 ? 1 : 0)
		end
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

	def _getv
		addr = _getp
		return if addr == 0
		@_tg._at(addr)._get
	end

	# XXX shaky...
	def _set(v)
		if v.kind_of?(Pointer)
			DFHack.memory_write_int32(@_memaddr, v._getv)
		elsif v.kind_of?(MemStruct)
			DFHack.memory_write_int32(@_memaddr, v._memaddr)
		else
			_getv._set(v)
		end
	end

	# these ruby Object methods should be forwarded to the ptr
	undef id
	undef type
	def method_missing(*a)
		_getv.send(*a)
	end
	def respond_to?(q)
		_getv.respond_to?(q)
	end

	def inspect
		cn = (@_tg ? @_tg.class.name.sub(/^DFHack::/, '') : '')
		cn = @_tg._glob if cn == 'MemHack::Global'
		"#<Pointer #{cn} #{'0x%X' % _getp}>"
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

	def inspect ; "#<PointerAry #{'0x%X' % _getp}>" ; end
end
module IndexEnum
	def indexenum(idx)
		if idx.kind_of?(::Symbol) or idx.kind_of?(::String) and _indexenum
			DFHack.const_get(_indexenum).const_get(idx) || idx
		else
			idx
		end
	end
end
class StaticArray < MemStruct
	include IndexEnum
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
	alias length _length
	alias size _length
	def _tgat(i)
		@_tg._at(@_memaddr + i*@_tglen) if i >= 0 and i < @_length
	end
	def [](i)
		i = indexenum(i)
		i += @_length if i < 0
		_tgat(i)._get
	end
	def []=(i, v)
		i = indexenum(i)
		i += @_length if i < 0
		_tgat(i)._set(v)
	end

	include Enumerable
	def each ; (0...length).each { |i| yield self[i] } ; end
	def inspect
		if _indexenum
 			e = DFHack.const_get(_indexenum)::ENUM
			'[' + (0...length).map { |i| "#{e[i]}=#{self[i].inspect}" }.join(' ') + ']'
		else
			to_a.inspect
		end
	end
	def flatten ; map { |e| e.respond_to?(:flatten) ? e.flatten : e }.flatten ; end
end
class StaticString < MemStruct
	attr_accessor :_length
	def initialize(length)
		@_length = length
	end
	def _get
		DFHack.memory_read(@_memaddr, @_length)
	end
	def _set(v)
		DFHack.memory_write(@_memaddr, v[0, @_length])
	end
end

class StlVector32 < MemStruct
	attr_accessor :_tg
	def initialize(tg)
		@_tg = tg
	end

	def length
		DFHack.memory_vector32_length(@_memaddr)
	end
	def size ; length ; end	# alias wouldnt work for subclasses
	def valueptr_at(idx)
		DFHack.memory_vector32_ptrat(@_memaddr, idx)
	end
	def insert_at(idx, val)
		DFHack.memory_vector32_insert(@_memaddr, idx, val)
	end
	def delete_at(idx)
		DFHack.memory_vector32_delete(@_memaddr, idx)
	end

	def _set(v)
		delete_at(length-1) while length > v.length	# match lengthes
		v.each_with_index { |e, i| self[i] = e }	# patch entries
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
			insert_at(idx, 0)
		elsif idx < 0
			raise 'invalid idx'
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
	def each ; (0...length).each { |i| yield self[i] } ; end
	def inspect
		if _tg and _tg.kind_of?(Pointer)
			length > 0 ? "[#{length}*#{self[0].inspect}]" : '[]'
		else
			to_a.inspect
		end
	end
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
		DFHack.memory_vector16_insert(@_memaddr, idx, val)
	end
	def delete_at(idx)
		DFHack.memory_vector16_delete(@_memaddr, idx)
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
		DFHack.memory_vector8_insert(@_memaddr, idx, val)
	end
	def delete_at(idx)
		DFHack.memory_vector8_delete(@_memaddr, idx)
	end
end
class StlBitVector < StlVector32
	def initialize ; end
	def length
		DFHack.memory_vectorbool_length(@_memaddr)
	end
	def insert_at(idx, val)
		DFHack.memory_vectorbool_insert(@_memaddr, idx, val)
	end
	def delete_at(idx)
		DFHack.memory_vectorbool_delete(@_memaddr, idx)
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
			raise 'invalid idx'
		else
			DFHack.memory_vectorbool_setat(@_memaddr, idx, v)
		end
	end
end
class StlString < MemStruct
	def _get
		DFHack.memory_read_stlstring(@_memaddr)
	end

	def _set(v)
		DFHack.memory_write_stlstring(@_memaddr, v)
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
	# possible struct size, like for StlVector. Just ignore it for now, deque are rare enough.
	def inspect ; "#<StlDeque>" ; end
end

class DfFlagarray < MemStruct
	include IndexEnum
	attr_accessor :_indexenum
	def initialize(indexenum)
		@_indexenum = indexenum
	end
	def length
		DFHack.memory_bitarray_length(@_memaddr)
	end
	# TODO _cpp_init
	def size ; length ; end
	def resize(len)
		DFHack.memory_bitarray_resize(@_memaddr, len)
	end
	def [](idx)
		idx = indexenum(idx)
		idx += length if idx < 0
		DFHack.memory_bitarray_isset(@_memaddr, idx) if idx >= 0 and idx < length
	end
	def []=(idx, v)
		idx = indexenum(idx)
		idx += length if idx < 0
		if idx >= length or idx < 0
			raise 'invalid idx'
		else
			DFHack.memory_bitarray_set(@_memaddr, idx, v)
		end
	end

	include Enumerable
	def each ; (0...length).each { |i| yield self[i] } ; end
	def inspect
		if _indexenum
 			e = DFHack.const_get(_indexenum)::ENUM
			'[' + (0...length).map { |i| if self[i] ; e[i] || i ; end }.compact.join(' ') + ']'
		else
			to_a.inspect
		end
	end
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
	# TODO _cpp_init
	def _tgat(i)
		@_tg._at(_ptr + i*@_tglen) if i >= 0 and i < _length
	end
	def [](i)
		i += _length if i < 0
		_tgat(i)._get
	end
	def []=(i, v)
		i += _length if i < 0
		_tgat(i)._set(v)
	end
	def _set(a)
		a.each_with_index { |v, i| self[i] = v }
	end

	include Enumerable
	def each ; (0...length).each { |i| yield self[i] } ; end
	def inspect ; to_a.inspect ; end
end
class DfLinkedList < Compound
	attr_accessor :_tg
	def initialize(tg)
		@_tg = tg
	end

	field(:_ptr, 0) { number 32, false }
	field(:_prev, 4) { number 32, false }
	field(:_next, 8) { number 32, false }

	def item
		addr = _ptr
		return if addr == 0
		@_tg._at(addr)._get
	end

	def item=(v)
		addr = _ptr
		raise 'null pointer' if addr == 0
		@_tg.at(addr)._set(v)
	end

	def prev
		addr = _prev
		return if addr == 0
		_at(addr)
	end

	def next
		addr = _next
		return if addr == 0
		_at(addr)
	end

	include Enumerable
	def each
		o = self
		while o
			yield o.item if o.item
			o = o.next
		end
	end

	def inspect ; "#<DfLinkedList prev=#{'0x%X' % _prev} next=#{'0x%X' % _next} #{item.inspect}>" ; end
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
end	# module MemHack


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
		n = @rtti_v2n[vptr] = get_rtti_classname(vptr)
		@rtti_n2v[n] = vptr
	end
	n
end

# return the vtable pointer from the cpp rtti name
def self.rtti_getvtable(cppname)
	unless v = @rtti_n2v[cppname]
		v = get_vtable(cppname)
		@rtti_n2v[cppname] = v
		@rtti_v2n[v] = cppname if v != 0
	end
	v if v != 0
end

end


