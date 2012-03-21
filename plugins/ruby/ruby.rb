module DFHack
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
	end

	def puts_err(*a)
		a.flatten.each { |l|
			print_err(l.to_s.chomp + "\n")
		}
	end

	def test
		puts "starting"

		suspend {
			c = cursor
			puts "cursor pos: #{c.x} #{c.y} #{c.z}"

			puts "unit[0] id: #{world.units.all[0].id}"
		}

		puts "done"
	end
end

end

# load user-specified startup file
load 'ruby_custom.rb' if File.exist?('ruby_custom.rb')
