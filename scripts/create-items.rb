# create first necessity items under cursor

category = $script_args[0] || 'help'
mat_raw  = $script_args[1] || 'list'
count    = $script_args[2]


category = df.match_rawname(category, ['help', 'bars', 'boulders', 'plants', 'logs', 'webs', 'anvils']) || 'help'

if category == 'help'
	puts <<EOS
Create first necessity items under the cursor.
Usage:
 create-items [category] [raws token] [number]

Item categories:
 bars, boulders, plants, logs, webs, anvils

Raw token:
 Either a full token (PLANT_MAT:ADLER:WOOD) or the middle part only
 (the missing part is autocompleted depending on the item category)
 Use 'list' to show all possibilities

Exemples:
 create-items boulders hematite 30
 create-items bars CREATURE_MAT:CAT:SOAP 10
 create-items web cave_giant
 create-items plants list

EOS
	throw :script_finished

elsif mat_raw == 'list'
	# allowed with no cursor

elsif df.cursor.x == -30000
	puts "Please place the game cursor somewhere"
	throw :script_finished

elsif !(maptile = df.map_tile_at(df.cursor))
	puts "Error: unallocated map block !"
	throw :script_finished

elsif !maptile.shape_passablehigh
	puts "Error: impassible tile !"
	throw :script_finished
end


def match_list(tok, list)
	if tok != 'list'
		tok = df.match_rawname(tok, list)
		if not tok
			puts "Invalid raws token, use one in:"
			tok = 'list'
		end
	end
	if tok == 'list'
		puts list.map { |w| w =~ /[^\w]/ ? w.inspect : w }.join(' ')
		throw :script_finished
	end
	tok
end


case category
when 'bars'
	# create metal bar, eg createbar INORGANIC:IRON
	cls = DFHack::ItemBarst
	if mat_raw !~ /:/ and !(df.decode_mat(mat_raw) rescue nil)
		list = df.world.raws.inorganics.find_all { |ino|
			ino.material.flags[:IS_METAL]
		}.map { |ino| ino.id }
		mat_raw = match_list(mat_raw, list)
		mat_raw = "INORGANIC:#{mat_raw}"
		puts mat_raw
	end
	customize = lambda { |item|
		item.dimension = 150
		item.subtype = -1
	}

when 'boulders'
	cls = DFHack::ItemBoulderst
	if mat_raw !~ /:/ and !(df.decode_mat(mat_raw) rescue nil)
		list = df.world.raws.inorganics.find_all { |ino|
			ino.material.flags[:IS_STONE]
		}.map { |ino| ino.id }
		mat_raw = match_list(mat_raw, list)
		mat_raw = "INORGANIC:#{mat_raw}"
		puts mat_raw
	end

when 'plants'
	cls = DFHack::ItemPlantst
	if mat_raw !~ /:/ and !(df.decode_mat(mat_raw) rescue nil)
		list = df.world.raws.plants.all.find_all { |plt|
			plt.material.find { |mat| mat.id == 'STRUCTURAL' }
		}.map { |plt| plt.id }
		mat_raw = match_list(mat_raw, list)
		mat_raw = "PLANT_MAT:#{mat_raw}:STRUCTURAL"
		puts mat_raw
	end

when 'logs'
	cls = DFHack::ItemWoodst
	if mat_raw !~ /:/ and !(df.decode_mat(mat_raw) rescue nil)
		list = df.world.raws.plants.all.find_all { |plt|
			plt.material.find { |mat| mat.id == 'WOOD' }
		}.map { |plt| plt.id }
		mat_raw = match_list(mat_raw, list)
		mat_raw = "PLANT_MAT:#{mat_raw}:WOOD"
		puts mat_raw
	end

when 'webs'
	cls = DFHack::ItemThreadst
	if mat_raw !~ /:/ and !(df.decode_mat(mat_raw) rescue nil)
		list = df.world.raws.creatures.all.find_all { |cre|
			cre.material.find { |mat| mat.id == 'SILK' }
		}.map { |cre| cre.creature_id }
		mat_raw = match_list(mat_raw, list)
		mat_raw = "CREATURE_MAT:#{mat_raw}:SILK"
		puts mat_raw
	end
	count ||= 1
	customize = lambda { |item|
		item.flags.spider_web = true
		item.dimension = 15000	# XXX may depend on creature (this is for GCS)
	}

when 'anvils'
	cls = DFHack::ItemAnvilst
	if mat_raw !~ /:/ and !(df.decode_mat(mat_raw) rescue nil)
		list = df.world.raws.inorganics.find_all { |ino|
			ino.material.flags[:IS_METAL]
		}.map { |ino| ino.id }
		mat_raw = match_list(mat_raw, list)
		mat_raw = "INORGANIC:#{mat_raw}"
		puts mat_raw
	end
	count ||= 1

end


mat = df.decode_mat mat_raw

count ||= 20
count.to_i.times {
	item = cls.cpp_new
	item.id = df.item_next_id
	item.stack_size = 1
	item.mat_type = mat.mat_type
	item.mat_index = mat.mat_index

	customize[item] if customize

	df.item_next_id += 1
	item.categorize(true)
	df.world.items.all << item

	item.pos = df.cursor
	item.flags.on_ground = true
	df.map_tile_at.mapblock.items << item.id
	df.map_tile_at.occupancy.item = true
}


# move game view, so that the ui menu updates
if df.cursor.z > 5
	df.curview.feed_keys(:CURSOR_DOWN_Z)
	df.curview.feed_keys(:CURSOR_UP_Z)
else
	df.curview.feed_keys(:CURSOR_UP_Z)
	df.curview.feed_keys(:CURSOR_DOWN_Z)
end
