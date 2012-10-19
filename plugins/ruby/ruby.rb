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
    class OnupdateCallback
        attr_accessor :callback, :timelimit, :minyear, :minyeartick
        def initialize(cb, tl, initdelay=0)
            @callback = cb
            @ticklimit = tl
            @minyear = (tl ? df.cur_year : 0)
            @minyeartick = (tl ? df.cur_year_tick+initdelay : 0)
        end

        # run callback if timedout
        def check_run(year, yeartick, yearlen)
            if !@ticklimit
                @callback.call
            else
                if year > @minyear or (year == @minyear and yeartick >= @minyeartick)
                    @minyear = year
                    @minyeartick = yeartick + @ticklimit
                    if @minyeartick > yearlen
                        @minyear += 1
                        @minyeartick -= yearlen
                    end
                    @callback.call
                end
            end
        rescue
            puts_err "onupdate cb #$!", $!.backtrace
        end

        def <=>(o)
            [@minyear, @minyeartick] <=> [o.minyear, o.minyeartick]
        end
    end

    class << self
        attr_accessor :onupdate_list, :onstatechange_list

        # register a callback to be called every gframe or more
        # ex: DFHack.onupdate_register { DFHack.world.units[0].counters.job_counter = 0 }
        def onupdate_register(ticklimit=nil, initialtickdelay=0, &b)
            @onupdate_list ||= []
            @onupdate_list << OnupdateCallback.new(b, ticklimit, initialtickdelay)
            DFHack.onupdate_active = true
            if onext = @onupdate_list.sort.first
                DFHack.onupdate_minyear = onext.minyear
                DFHack.onupdate_minyeartick = onext.minyeartick
            end
            @onupdate_list.last
        end

        # delete the callback for onupdate ; use the value returned by onupdate_register
        def onupdate_unregister(b)
            @onupdate_list.delete b
            if @onupdate_list.empty?
                DFHack.onupdate_active = false
                DFHack.onupdate_minyear = DFHack.onupdate_minyeartick = 0
            end
        end

        # same as onupdate_register, but remove the callback once it returns true
        def onupdate_register_once(*a)
            handle = onupdate_register(*a) {
                onupdate_unregister(handle) if yield
            }
        end

        TICKS_PER_YEAR = 1200*28*12
        # this method is called by dfhack every 'onupdate' if onupdate_active is true
        def onupdate
            @onupdate_list ||= []

            ticks_per_year = TICKS_PER_YEAR
            ticks_per_year *= 72 if gametype == :ADVENTURE_MAIN or gametype == :ADVENTURE_ARENA

            @onupdate_list.each { |o|
                o.check_run(cur_year, cur_year_tick, ticks_per_year)
            }

            if onext = @onupdate_list.sort.first
                DFHack.onupdate_minyear = onext.minyear
                DFHack.onupdate_minyeartick = onext.minyeartick
            end
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

        # same as onstatechange_register, but auto-unregisters if the block returns true
        def onstatechange_register_once
            handle = onstatechange_register { |st|
                onstatechange_unregister(handle) if yield(st)
            }
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

        def translate_name(name, english=true, onlylastpart=false)
            out = []

            if not onlylastpart
                out << name.first_name if name.first_name != ''
                if name.nickname != ''
                    case respond_to?(:d_init) && d_init.nickname_dwarf
                    when :REPLACE_ALL; return "`#{name.nickname}'"
                    when :REPLACE_FIRST; out.pop
                    end
                    out << "`#{name.nickname}'"
                end
            end
            return out.join(' ') unless name.words.find { |w| w >= 0 }

            if not english
                tsl = world.raws.language.translations[name.language]
                if name.words[0] >= 0 or name.words[1] >= 0
                    out << ''
                    out.last << tsl.words[name.words[0]] if name.words[0] >= 0
                    out.last << tsl.words[name.words[1]] if name.words[1] >= 0
                end
                if name.words[5] >= 0
                    out << ''
                    (2..5).each { |i| out.last << tsl.words[name.words[i]] if name.words[i] >= 0 }
                end
                if name.words[6] >= 0
                    out << tsl.words[name.words[6]]
                end
            else
                wl = world.raws.language
                if name.words[0] >= 0 or name.words[1] >= 0
                    out << ''
                    out.last << wl.words[name.words[0]].forms[name.parts_of_speech[0]] if name.words[0] >= 0
                    out.last << wl.words[name.words[1]].forms[name.parts_of_speech[1]] if name.words[1] >= 0
                end
                if name.words[5] >= 0
                    out << 'the '
                    out.last.capitalize! if out.length == 1
                    (2..5).each { |i| out.last << wl.words[name.words[i]].forms[name.parts_of_speech[i]] if name.words[i] >= 0 }
                end
                if name.words[6] >= 0
                    out << 'of'
                    out.last.capitalize! if out.length == 1
                    out << wl.words[name.words[6]].forms[name.parts_of_speech[6]]
                end
            end

            out.join(' ')
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
