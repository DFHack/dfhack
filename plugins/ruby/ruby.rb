module DFHack
    class << self
        # update the ruby.cpp version to accept a block
        def suspend
            if block_given?
                begin
                    do_suspend
                    yield
                ensure
                    resume
                end
            else
                do_suspend
            end
        end

        module ::Kernel
            def puts(*a)
                a.flatten.each { |l|
                    DFHack.print_str(l.to_s.chomp + "\n")
                }
                nil
            end

            def puts_err(*a)
                a.flatten.each { |l|
                    DFHack.print_err(l.to_s.chomp + "\n")
                }
                nil
            end

            def p(*a)
                a.each { |e|
                    puts_err e.inspect
                }
            end
        end

        # register a callback to be called every gframe or more
        # ex: DFHack.onupdate_register { DFHack.world.units[0].counters.job_counter = 0 }
        def onupdate_register(&b)
            @onupdate_list ||= []
            @onupdate_list << b
            DFHack.onupdate_active = true
            @onupdate_list.last
        end

        # delete the callback for onupdate ; use the value returned by onupdate_register
        def onupdate_unregister(b)
            @onupdate_list.delete b
            DFHack.onupdate_active = false if @onupdate_list.empty?
        end

        # this method is called by dfhack every 'onupdate' if onupdate_active is true
        def onupdate
            @onupdate_list ||= []
            @onupdate_list.each { |cb| cb.call }
        end

        # register a callback to be called every gframe or more
        # ex: DFHack.onstatechange_register { |newstate| puts "state changed to #{newstate}" }
        def onstatechange_register(&b)
            @onstatechange_list ||= []
            @onstatechange_list << b
            @onstatechange_list.last
        end

        # delete the callback for onstatechange ; use the value returned by onstatechange_register
        def onstatechange_unregister(b)
            @onstatechange_list.delete b
        end

        # this method is called by dfhack every 'onstatechange'
        def onstatechange(newstate)
            @onstatechange_list ||= []
            @onstatechange_list.each { |cb| cb.call(newstate) }
        end


        # return an Unit
        # with no arg, return currently selected unit in df UI ('v' or 'k' menu)
        # with numeric arg, search unit by unit.id
        # with an argument that respond to x/y/z (eg cursor), find first unit at this position
        def find_unit(what=:selected)
            if what == :selected
                case ui.main.mode
                when :ViewUnits
                    # nobody selected => idx == 0
                    v = world.units.active[ui_selected_unit]
                    v if v and v.pos.z == cursor.z
                when :LookAround
                    k = ui_look_list.items[ui_look_cursor]
                    k.unit if k.type == :Unit
                end
            elsif what.kind_of?(Integer)
                world.units.all.binsearch(what)
            elsif what.respond_to?(:x) or what.respond_to?(:pos)
                what = what.pos if what.respond_to?(:pos)
                x, y, z = what.x, what.y, what.z
                world.units.all.find { |u| u.pos.x == x and u.pos.y == y and u.pos.z == z }
            else
                raise "what what?"
            end
        end

        # return an Item
        # arg similar to find_unit; no arg = 'k' menu
        def find_item(what=:selected)
            if what == :selected
                case ui.main.mode
                when :LookAround
                    k = ui_look_list.items[ui_look_cursor]
                    k.item if k.type == :Item
                end
            elsif what.kind_of?(Integer)
                world.items.all.binsearch(what)
            elsif what.respond_to?(:x) or what.respond_to?(:pos)
                what = what.pos if what.respond_to?(:pos)
                x, y, z = what.x, what.y, what.z
                world.items.all.find { |i| i.pos.x == x and i.pos.y == y and i.pos.z == z }
            else
                raise "what what?"
            end
        end

        # return a map block by tile coordinates
        # you can also use find_map_block(cursor) or anything that respond to x/y/z
        def map_block_at(x, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)
            if x >= 0 and x < world.map.x_count and y >= 0 and y < world.map.y_count and z >= 0 and z < world.map.z_count
                world.map.block_index[x/16][y/16][z]
            end
        end

        def map_designation_at(x, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)
            if b = map_block_at(x, y, z)
                b.designation[x%16][y%16]
            end
        end

        def map_occupancy_at(x, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)
            if b = map_block_at(x, y, z)
                b.occupancy[x%16][y%16]
            end
        end

        # yields every map block
        def each_map_block
            (0...world.map.x_count_block).each { |xb|
                xl = world.map.block_index[xb]
                (0...world.map.y_count_block).each { |yb|
                    yl = xl[yb]
                    (0...world.map.z_count_block).each { |z|
                        p = yl[z]
                        yield p if p
                    }
                }
            }
        end

        # yields every map block for a given z level
        def each_map_block_z(z)
            (0...world.map.x_count_block).each { |xb|
                xl = world.map.block_index[xb]
                (0...world.map.y_count_block).each { |yb|
                    p = xl[yb][z]
                    yield p if p
                }
            }
        end

        # return true if the argument is under the cursor
        def at_cursor?(obj)
            same_pos?(obj, cursor)
        end

        # returns true if both arguments are at the same x/y/z
        def same_pos?(pos1, pos2)
            pos1 = pos1.pos if pos1.respond_to?(:pos)
            pos2 = pos2.pos if pos2.respond_to?(:pos)
            pos1.x == pos2.x and pos1.y == pos2.y and pos1.z == pos2.z
        end

        # center the DF screen on something
        # updates the cursor position if visible
        def center_viewscreen(x, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)

            # compute screen 'map' size (tiles)
            menuwidth = ui_menu_width
            # ui_menu_width shows only the 'tab' status
            menuwidth = 1 if menuwidth == 2 and ui_area_map_width == 2 and cursor.x != -30000
            menuwidth = 2 if menuwidth == 3 and cursor.x != -30000
            w_w = gps.dimx - 2
            w_h = gps.dimy - 2
            case menuwidth
            when 1; w_w -= 55
            when 2; w_w -= (ui_area_map_width == 2 ? 24 : 31)
            end

            # center view
            w_x = x - w_w/2
            w_y = y - w_h/2
            w_z = z
            # round view coordinates (optional)
            #w_x -= w_x % 10
            #w_y -= w_y % 10
            # crop to map limits
            w_x = [[w_x, world.map.x_count - w_w].min, 0].max
            w_y = [[w_y, world.map.y_count - w_h].min, 0].max

            self.window_x = w_x
            self.window_y = w_y
            self.window_z = w_z

            if cursor.x != -30000
                cursor.x, cursor.y, cursor.z = x, y, z
            end
        end

        # add an announcement
        # color = integer, bright = bool
        def add_announcement(str, color=nil, bright=nil)
            cont = false
            while str.length > 0
                rep = Report.cpp_new
                rep.color = color if color
                rep.bright = ((bright && bright != 0) ? 1 : 0) if bright != nil
                rep.year = cur_year
                rep.time = cur_year_tick
                rep.flags.continuation = cont
                cont = true
                rep.flags.announcement = true
                rep.text = str[0, 73]
                str = str[73..-1].to_s
                rep.id = world.status.next_report_id
                world.status.next_report_id += 1
                world.status.reports << rep
                world.status.announcements << rep
                world.status.display_timer = 2000
            end
        end

        # try to match a user-specified name to one from the raws
        # uses case-switching and substring matching
        # eg match_rawname('coal', ['COAL_BITUMINOUS', 'BAUXITE']) => 'COAL_BITUMINOUS'
        def match_rawname(name, rawlist)
            rawlist.each { |r| return r if name == r }
            rawlist.each { |r| return r if name.downcase == r.downcase }
            may = rawlist.find_all { |r| r.downcase.index(name.downcase) }
            may.first if may.length == 1
        end

        # link a job to the world
        # allocate & set job.id, allocate a JobListLink, link to job & world.job_list
        def job_link(job)
            lastjob = world.job_list
            lastjob = lastjob.next while lastjob.next
            joblink = JobListLink.cpp_new
            joblink.prev = lastjob
            joblink.item = job
            job.list_link = joblink
            job.id = df.job_next_id
            df.job_next_id += 1
            lastjob.next = joblink
        end

        # attach an item to a job, flag item in_job
        def job_attachitem(job, item, role=:Hauled, filter_idx=-1)
            if role != :TargetContainer
                item.flags.in_job = true
            end

            itemlink = SpecificRef.cpp_new
            itemlink.type = :JOB
            itemlink.job = job
            item.specific_refs << itemlink

            joblink = JobItemRef.cpp_new
            joblink.item = item
            joblink.role = role
            joblink.job_item_idx = filter_idx
            job.items << joblink
        end
    end
end

# global alias so we can write 'df.world.units.all[0]'
def df
    DFHack
end

# following: definitions used by ruby-autogen.rb
module DFHack
module MemHack
INSPECT_SIZE_LIMIT=16384
class MemStruct
	attr_accessor :_memaddr
	def _at(addr) ; d = dup ; d._memaddr = addr ; d ; end
	def _get ; self ; end
	def _cpp_init ; end
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
		def bit(shift)
			BitField.new(shift, 1)
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
	def _set(h)
		case h
		when Hash; h.each { |k, v| send("_#{k}=", v) }
		when Array; names = _field_names ; raise 'bad size' if names.length != h.length ; names.zip(h).each { |n, a| send("#{n}=", a) }
		when Compound; _field_names.each { |n| send("#{n}=", h.send(n)) }
		else raise 'wut?'
		end
	end
	def _fields ; self.class._fields.to_a ; end
	def _fields_ancestors ; self.class._fields_ancestors.to_a ; end
	def _field_names ; _fields_ancestors.map { |n, o, s| n } ; end
	def _rtti_classname ; self.class._rtti_classname ; end
	def _sizeof ; self.class._sizeof ; end
	@@inspecting = {}	# avoid infinite recursion on mutually-referenced objects
	def inspect
		cn = self.class.name.sub(/^DFHack::/, '')
		cn << ' @' << ('0x%X' % _memaddr) if cn != ''
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
		if s.kind_of?(BitField) and s._len == 1
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

	def self.int(i)
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
		if @_len == 1
			((v & 1) == 0) ? false : true
		else
			v &= _mask
			v = @_enum.sym(v) if @_enum
			v
		end
	end

	def _set(v)
		if @_len == 1
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

	def _get
		addr = _getp
		return if addr == 0
		@_tg._at(addr)._get
	end

	# XXX shaky...
	def _set(v)
		if v.kind_of?(Pointer)
			DFHack.memory_write_int32(@_memaddr, v._getp)
		elsif v.kind_of?(MemStruct)
			DFHack.memory_write_int32(@_memaddr, v._memaddr)
		else
			_get._set(v)
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
	alias length _length
	alias size _length
	def _tgat(i)
		@_tg._at(@_memaddr + i*@_tglen) if i >= 0 and i < @_length
	end
	def [](i)
		i = _indexenum.int(i) if _indexenum
		i += @_length if i < 0
		_tgat(i)._get
	end
	def []=(i, v)
		i = _indexenum.int(i) if _indexenum
		i += @_length if i < 0
		_tgat(i)._set(v)
	end

	include Enumerable
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
		idx = _indexenum.int(idx) if _indexenum
		idx += length if idx < 0
		DFHack.memory_bitarray_isset(@_memaddr, idx) if idx >= 0 and idx < length
	end
	def []=(idx, v)
		idx = _indexenum.int(idx) if _indexenum
		idx += length if idx < 0
		if idx >= length or idx < 0
			raise 'invalid idx'
		else
			DFHack.memory_bitarray_set(@_memaddr, idx, v)
		end
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
		# With the current xml structure, currently _tg designate
		# the type of the 'next' and 'prev' fields, not 'item'.
		# List head has item == NULL, so we can safely return nil.

		#addr = _ptr
		#return if addr == 0
		#@_tg._at(addr)._get
	end

	def item=(v)
		#addr = _ptr
		#raise 'null pointer' if addr == 0
		#@_tg.at(addr)._set(v)
		raise 'null pointer'
	end

	def prev
		addr = _prev
		return if addr == 0
		@_tg._at(addr)._get
	end

	def next
		addr = _next
		return if addr == 0
		@_tg._at(addr)._get
	end

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
end	# module MemHack

class BooleanEnum
    def self.int(v) ; ((v == true) || (v == 1)) ? 1 : 0 ; end
    def self.sym(v) ; (!v || (v == 0)) ? false : true ; end
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

def self.vmethod_call(obj, voff, a0=0, a1=0, a2=0, a3=0, a4=0)
	vmethod_do_call(obj._memaddr, voff, vmethod_arg(a0), vmethod_arg(a1), vmethod_arg(a2), vmethod_arg(a3))
end

def self.vmethod_arg(arg)
	case arg
	when nil, false; 0
	when true; 1
	when Integer; arg
	#when String; [arg].pack('p').unpack('L')[0]	# raw pointer to buffer
	when MemHack::Compound; arg._memaddr
	else raise "bad vmethod arg #{arg.class}"
	end
end

end

# load autogen'd file
File.exist?('hack/ruby-autogen.rb') ? require('hack/ruby-autogen') : require('ruby-autogen')

# load optional user-specified startup file
load 'ruby_custom.rb' if File.exist?('ruby_custom.rb')
