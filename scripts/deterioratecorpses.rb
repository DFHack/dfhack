# Make corpse parts decay and vanish over time
=begin

deterioratecorpses
==================
Somewhere between a "mod" and a "fps booster", with a small impact on
vanilla gameplay.

In long running forts, especially evil biomes, you end up with a lot
of toes, teeth, fingers, and limbs scattered all over the place.
Various corpses from various sieges, stray kitten corpses, probably
some heads. Basically, your map will look like a giant pile of
assorted body parts, all of which individually eat up a small part
of your FPS, which collectively eat up quite a bit.

In addition, this script also targets various butchery byproducts.
Enjoying your thriving animal industry? Your FPS does not. Those
thousands of skulls, bones, hooves, and wool eat up precious FPS
that could be used to kill goblins and elves. Whose corpses will
also get destroyed by the script to kill more goblins and elves.

This script causes all of those to rot away into nothing after
several months.

Usage: ``deterioratecorpses (start|stop)``

=end

class DeteriorateCorpses

    def initialize
    end

    def process
        return false unless @running

        df.world.items.other[:ANY_CORPSE].each { |i|
            if (i.flags.dead_dwarf == false)
                i.wear_timer += 1
                if (i.wear_timer > 24 + rand(8))
                    i.wear_timer = 0
                    i.wear += 1
                end
                if (i.wear > 3)
                    i.flags.garbage_collect = true
                end

            end

        }

        df.world.items.other[:REMAINS].each { |i|
            if (i.flags.dead_dwarf == false)
                i.wear_timer += 1
                if (i.wear_timer > 6)
                    i.wear_timer = 0
                    i.wear += 1
                end
                if (i.wear > 3)
                    i.flags.garbage_collect = true
                end

            end

        }

    end

    def start
        @onupdate = df.onupdate_register('deterioratecorpses', 1200, 1200) { process }
        @running = true

        puts "Deterioration of body parts commencing..."

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
    if ($DeteriorateCorpses)
        $DeteriorateCorpses.stop
    end

    $DeteriorateCorpses = DeteriorateCorpses.new
    $DeteriorateCorpses.start

when 'end', 'stop'
    $DeteriorateCorpses.stop

else
    if $DeteriorateCorpses
        puts $DeteriorateCorpses.status
    else
        puts 'Not loaded.'
    end
end