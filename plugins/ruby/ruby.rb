require 'hack/ruby-autogen'

module DFHack
    class << self
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

        def puts(*a)
            a.flatten.each { |l|
                print_str(l.to_s.chomp + "\n")
            }
            nil
        end

        def puts_err(*a)
            a.flatten.each { |l|
                print_err(l.to_s.chomp + "\n")
            }
            nil
        end

        def test
            puts "starting"

            suspend {
                puts "cursor pos: #{cursor.x} #{cursor.y} #{cursor.z}"

                puts "unit[0] id: #{world.units.all[0].id}"

                if cursor.x >= 0
                    world.map.block_index[cursor.x/16][cursor.y/16][cursor.z].designation[cursor.x%16][cursor.y%16].dig = TileDigDesignation::Default
                    puts "dug cursor tile"
                end
            }

            puts "done"
        end
    end
end

# load user-specified startup file
load 'ruby_custom.rb' if File.exist?('ruby_custom.rb')
