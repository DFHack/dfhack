module DFHack
def self.buildbedhere
	suspend {
		raise 'where to ?' if cursor.x < 0

		item = world.items.all.find { |i|
			i.kind_of?(ItemBedst) and
			i.itemrefs.empty? and
			!i.flags.in_job
		}
		raise 'no free bed, build more !' if not item

		job = Job.cpp_new
		jobitemref = JobItemRef.cpp_new
		refbuildingholder = GeneralRefBuildingHolderst.cpp_new
		building = BuildingBedst.cpp_new
		itemjob = SpecificRef.cpp_new

		buildingid = df.building_next_id
		df.building_next_id += 1

		job.job_type = :ConstructBuilding
		job.pos = cursor
		job.mat_type = item.mat_type
		job.mat_index = item.mat_index
		job.hist_figure_id = -1
		job.items << jobitemref
		job.references << refbuildingholder

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
		building.specific_squad = -1

		itemjob.type = :JOB
		itemjob.job = job

		item.flags.in_job = true
		item.specific_refs << itemjob

		map_block_at(cursor).occupancy[cursor.x%16][cursor.y%16].building = :Planned
		link_job job
		world.buildings.all << building
		building.categorize(true)
	}
end
end
