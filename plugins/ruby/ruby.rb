# redefine standard i/o methods to use the dfhack console
module Kernel
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
        nil
    end
end

module DFHack
    class << self
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

        # try to match a user-specified name to one from the raws
        # uses case-switching and substring matching
        # eg match_rawname('coal', ['COAL_BITUMINOUS', 'BAUXITE']) => 'COAL_BITUMINOUS'
        def match_rawname(name, rawlist)
            rawlist.each { |r| return r if name == r }
            rawlist.each { |r| return r if name.downcase == r.downcase }
            may = rawlist.find_all { |r| r.downcase.index(name.downcase) }
            may.first if may.length == 1
        end
    end
end

# global alias so we can write 'df.world.units.all[0]'
def df
    DFHack
end

# load autogenned file
require './hack/ruby/ruby-autogen-defs'
require './hack/ruby/ruby-autogen'

# load all modules
Dir['./hack/ruby/*.rb'].each { |m| require m.chomp('.rb') }
