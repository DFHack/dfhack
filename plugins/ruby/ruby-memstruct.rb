class MemStruct
	attr_accessor :_memaddr
	def _at(addr) ; @_memaddr = addr ; self ; end
	def _get ; self ; end
end

class Compound < MemStruct
	class << self
		attr_accessor :_fields
		def field(name, offset)
			struct = yield
			@_fields ||= []
			@_fields << [name, offset, struct]
			define_method(name) { struct._at(@_memaddr+offset)._get }
			define_method("#{name}=") { |v| struct._at(@_memaddr+offset)._set(v) }
		end

		def number(bits, signed)
			Number.new(bits, signed)
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
		def pointer(tglen=nil)
			Pointer.new(tglen, (yield if tglen))
		end
		def static_array(len, tglen)
			StaticArray.new(tglen, len, yield)
		end
		def static_string(len)
			StaticString.new(len)
		end

		def stl_vector(tglen=nil)
			StlVector.new(tglen, (yield if tglen))
		end
		def stl_string
			StlString.new
		end
		def stl_bit_vector
			StlBitVector.new
		end
		def stl_deque(tglen=nil)
			StlDeque.new(tglen, (yield if tglen))
		end

		def df_flagarray
			DfFlagarray.new
		end
		def df_array(tglen=nil)
			DfArray.new(tglen, (yield if tglen))
		end
		def df_linked_list
			DfLinkedList.new((yield if block_given?))
		end

		def global(glob)
			Global.new(glob)
		end
		def compound(&b)
			m = Class.new(Compound)
			m.instance_eval(&b)
			m
		end
	end
	def _set(h) ; h.each { |k, v| send("_#{k}=", v) } ; end
end
class Number < MemStruct
	attr_accessor :_bits, :_signed
	def initialize(bits, signed)
		@_bits = bits
		@_signed = signed
	end
end
class Float < MemStruct
end
class BitField < Number
	attr_accessor :_shift, :_len
	def initialize(shift, len)
		@_shift = shift
		@_len = len
		super(32, false)
	end

	def _get(whole=false)
		v = super()
		return v if whole
		v = (v >> @_shift) % (1 << @_len)
		if @_len == 1
			v == 0 ? false : true
		else
			v
		end
	end

	def _set(v)
		if @_len == 1
			v = (v && v != 0 ? 1 : 0)
		end
		v = ((v % (1 << @_len)) << @_shift)

		ori = _get(true)
		super(ori - (ori & (-1 % (1 << @_len)) << @_shift) + v)
	end
end

class Pointer < MemStruct
	attr_accessor :_tglen, :_tg
	def initialize(tglen, tg)
		@_tglen = tglen
		@_tg = tg
	end
end
class StaticArray < MemStruct
	attr_accessor :_tglen, :_length, :_tg
	def initialize(tglen, length, tg)
		@_tglen = tglen
		@_length = length
		@_tg = tg
	end
	def _set(a) ; a.each_with_index { |v, i| self[i] = v } ; end
	alias length _length
	alias size _length
	def _tgat(i)
		tg._at(@_memaddr + i*@_tglen)
	end
	def [](i)
		if (i > 0) or (@_length and i < @_length)
			tgat(i)._get
		end
	end
	def []=(i, v)
		if (i > 0) or (@_length and i < @_length)
			tgat(i)._set
		end
	end
end
class StaticString < MemStruct
	attr_accessor :_length
	def initialize(length)
		@_length = length
	end
end

class StlVector < MemStruct
	attr_accessor :_tglen, :_tg
	def initialize(tglen, tg)
		@_tglen = tglen
		@_tg = tg
	end
end
class StlString < MemStruct
end
class StlBitVector < MemStruct
end
class StlDeque < MemStruct
	attr_accessor :_tglen, :_tg
	def initialize(tglen, tg)
		@_tglen = tglen
		@_tg = tg
	end
end

class DfFlagarray < MemStruct
end
class DfArray < MemStruct
	attr_accessor :_tglen, :_tg
	def initialize(tglen, tg)
		@_tglen = tglen
		@_tg = tg
	end
end
class DfLinkedList < MemStruct
	attr_accessor :_tg
	def initialize(tg)
		@_tg = tg
	end
end

class Global < MemStruct
	attr_accessor :_glob
	def initialize(glob)
		@_glob = glob
	end
	def _at(addr) ; g = const_get(@_glob) ; g._at(addr) ; end
end



