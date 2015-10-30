# Make undead units weaken after one month, and vanish after six
=begin

starvingdead
============
Somewhere between a "mod" and a "fps booster", with a small impact on
vanilla gameplay. It mostly helps prevent undead cascades in the caverns,
where constant combat leads to hundreds of undead roaming the
caverns and destroying your FPS.

With this script running, all undead that have been on the map for
one month gradually decay, losing strength, speed, and toughness.
After six months, they collapse upon themselves, never to be reanimated.

Usage: ``starvingdead (start|stop)``

=end

class StarvingDead

    def initialize
        @threshold = 1
        @die_threshold = 6
    end

    def process
        return false unless @running
        month_length = 67200
        if (@undead_count >= 25)
            month_length *= 25 / @undead_count
        end

        @undead_count = 0
        df.world.units.active.each { |u|
            if (u.enemy.undead and not u.flags1.dead)
                @undead_count += 1
                if (u.curse.time_on_site > month_length * @threshold)
                    u.body.physical_attrs.each { |att|
                        att.value = att.value - (att.value * 0.02)
                    }
                end

                if (u.curse.time_on_site > month_length * @die_threshold)
                    u.flags1.dead = true
                    u.curse.rem_tags2.FIT_FOR_ANIMATION = true
                end
            end
        }
    end

    def start
        @onupdate = df.onupdate_register('starvingdead', 1200, 1200) { process }
        @running = true
        @undead_count = 0

        if ($script_args[1] and $script_args[1].gsub(/[^0-9\.]/,'').to_f > 0)
            @threshold = $script_args[1].gsub(/[^0-9\.]/,'').to_f
        end

        if ($script_args[2] and $script_args[2].gsub(/[^0-9\.]/,'').to_f > 0)
            @die_threshold = $script_args[2].gsub(/[^0-9\.]/,'').to_f
        end

        puts "Starving Dead starting...weakness starts at #{@threshold} months, true death at #{@die_threshold} months"
    end

    def stop
        df.onupdate_unregister(@onupdate)
        @running = false
    end
    def status
        @running ? 'Running.' : 'Stopped.'
    end
end

case $script_args[0]
when 'start'
    if ($StarvingDead)
        $StarvingDead.stop
    end
    $StarvingDead = StarvingDead.new
    $StarvingDead.start

when 'end', 'stop'
    $StarvingDead.stop
else
    if $StarvingDead
        puts $StarvingDead.status
    else
        puts 'Not loaded.'
    end
end
