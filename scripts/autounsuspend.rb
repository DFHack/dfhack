class AutoUnsuspend

	def initialize
	end

	def process
		return false unless @running
		joblist = df.world.job_list.next
		count = 0

		while joblist
			job = joblist.item
			joblist = joblist.next
			
			if job.job_type == :ConstructBuilding
				if (job.flags.suspend)
					item = job.items[0].item
					job.flags.suspend = false
					count += 1
				end
			end
		end

		puts "Unsuspended #{count} job(s)." unless count == 0
		
	end
	
	def start
		@onupdate = df.onupdate_register('autounsuspend', 5) { process }
		@running = true
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
    $AutoUnsuspend = AutoUnsuspend.new unless $AutoUnsuspend
    $AutoUnsuspend.start

when 'end', 'stop'
    $AutoUnsuspend.stop
	
else
    if $AutoUnsuspend
        puts $AutoUnsuspend.status
    else
        puts 'Not loaded.'
    end
end
