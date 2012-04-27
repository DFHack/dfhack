module DFHack
def self.buildbedhere
	suspend {
		raise 'where to ?' if cursor.x < 0

		# TODO tweak that more
		item = world.items.all.find { |i|
			i.kind_of?(ItemBedst) and
			i.itemrefs.empty? and
			i.jobs.empty?
		}
		raise 'no free bed, build more !' if not item

		puts 'alloc c++'
		job = Job.cpp_new
		joblink = JobListLink.cpp_new
		jobitemref = JobItemRef.cpp_new
		refbuildingholder = GeneralRefBuildingHolderst.cpp_new
		building = BuildingBedst.cpp_new
		itemtjobs = Item_TJobs.cpp_new

		puts 'update global counters'
		# TODO
		buildingid = world.buildings.all[-1].id + 1
		jobid = df.job_next_id
		df.job_next_id = jobid+1

		puts 'init objects'
		job.id = jobid
		job.list_link = joblink
		job.job_type = :ConstructBuilding
		job.unk2 = -1	# XXX
		job.pos = cursor
		job.completion_timer = -1
		# unk4a/4b uninit
		job.mat_type = item.mat_type
		job.mat_index = item.mat_index
		job.unk5 = -1
		job.unk6 = -1
		job.item_subtype = -1
		job.hist_figure_id = -1
		job.items << jobitemref
		job.references << refbuildingholder

		lastjob = world.job_list
		lastjob = lastjob.next while lastjob.next
		lastjob.next = joblink
		joblink.prev = lastjob
		joblink.item = job

		jobitemref.item = item
		jobitemref.role = :Hauled
		jobitemref.job_item_idx = -1
		
		refbuildingholder.building_id = buildingid

		building.x1 = building.x2 = building.centerx = cursor.x
		building.y1 = building.y2 = building.centery = cursor.y
		building.z = cursor.z
		building.mat_type = item.mat_type
		building.mat_index = item.mat_index
		building.race = ui.race_id
		building.id = buildingid
		building.jobs << job
		building.anon_3 = -1
		building.anon_4 = -1

		itemtjobs.unk1 = 2
		itemtjobs.job = job

		item.flags.in_job = true
		item.jobs << itemtjobs

		puts 'update vectors'
		world.buildings.all << building
		# XXX
		world.buildings.other[0] << building
		world.buildings.other[4] << building
		world.buildings.other[8] << building
		world.buildings.other[9] << building
		world.buildings.other[10] << building
		world.buildings.other[32] << building
		# not in bad

	}
end
end
