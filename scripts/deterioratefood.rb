# Make food and plants decay, and vanish after a few months
=begin

deterioratefood
===============
Somewhere between a "mod" and a "fps booster", with a small impact on
vanilla gameplay.

With this script running, all food and plants wear out and disappear
after several months. Barrels and stockpiles will keep them from
rotting, but it won't keep them from decaying. No more sitting on a
hundred years worth of food. No more keeping barrels of pig tails
sitting around until you decide to use them. Either use it, eat it,
or lose it. Seeds, are excluded from this, if you aren't planning on
using your pig tails, hold onto the seeds for a rainy day.

This script is...pretty far reaching. However, almost all long
running forts I've had end up sitting on thousands and thousands of
food items. Several thousand cooked meals, three thousand plump
helmets, just as many fish and meat. It gets pretty absurd. And your
FPS doesn't like it.

Usage: ``deterioratefood (start|stop)``

=end

class DeteriorateFood

    def initialize
    end

    def process
        return false unless @running

        items = [df.world.items.other[:FISH],
                 df.world.items.other[:FISH_RAW],
                 df.world.items.other[:EGG],
                 df.world.items.other[:CHEESE],
                 df.world.items.other[:PLANT],
                 df.world.items.other[:PLANT_GROWTH],
                 df.world.items.other[:FOOD]]

        items.each { |type|
            type.each { |i|
                i.wear_timer += 1
                if (i.wear_timer > 24 + rand(8))
                    i.wear_timer = 0
                    i.wear += 1
                end
                if (i.wear > 3)
                    i.flags.garbage_collect = true
                end
            }
        }


    end

    def start
        @onupdate = df.onupdate_register('deterioratefood', 1200, 1200) { process }
        @running = true

        puts "Deterioration of food commencing..."

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
    if ($DeteriorateFood)
        $DeteriorateFood.stop
    end

    $DeteriorateFood = DeteriorateFood.new
    $DeteriorateFood.start

when 'end', 'stop'
    $DeteriorateFood.stop

else
    if $DeteriorateFood
        puts $DeteriorateFood.status
    else
        puts 'Not loaded.'
    end
end