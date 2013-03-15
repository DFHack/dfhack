joblist = df.world.job_list.next
count = 0

while joblist
	job = joblist.item
	joblist = joblist.next
	
	if job.job_type == :ConstructBuilding
		if (job.flags.suspend && job.items && job.items[0])
			item = job.items[0].item
			job.flags.suspend = false
			count += 1
		end
	end
end

puts "Unsuspended #{count} job(s)."
