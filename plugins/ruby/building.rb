module DFHack
    class << self
        def building_find(what=:selected, y=nil, z=nil)
            if what == :selected
                case ui.main.mode
                when :LookAround
                    k = ui_look_list.items[ui_look_cursor]
                    k.building if k.type == :Building
                when :BuildingItems, :QueryBuilding
                    world.selected_building
                when :Zones, :ZonesPenInfo, :ZonesPitInfo, :ZonesHospitalInfo
                    ui_sidebar_menus.zone.selected
                end

            elsif what.kind_of?(Integer)
                # search by building.id
                return world.buildings.all.binsearch(what) if not z

                # search by coordinates
                x = what
                world.buildings.all.find { |b|
                    b.z == z and
                    if b.room.extents
                        dx = x - b.room.x
                        dy = y - b.room.y
                        dx >= 0 and dx < b.room.width and
                        dy >= 0 and dy < b.room.height and
                        b.room.extents[ dy*b.room.width + dx ] > 0
                    else
                        b.x1 <= x and b.x2 >= x and
                        b.y1 <= y and b.y2 >= y
                    end
                }

            elsif what.respond_to?(:x) or what.respond_to?(:pos)
                # find the building at the same position
                what = what.pos if what.respond_to?(:pos)
                building_find(what.x, what.y, what.z)

            else
                raise "what what?"
            end
        end

        # allocate a new building object
        def building_alloc(type, subtype=-1, custom=-1)
            cls = rtti_n2c[BuildingType::Classname[type].to_sym]
            raise "invalid building type #{type.inspect}" if not cls
            bld = cls.cpp_new
            bld.race = ui.race_id
            subtype = ConstructionType.int(subtype) if subtype.kind_of?(::Symbol) and type == :Construction
            subtype = SiegeengineType.int(subtype) if subtype.kind_of?(::Symbol) and type == :SiegeEngine
            subtype = WorkshopType.int(subtype) if subtype.kind_of?(::Symbol) and type == :Workshop
            subtype = FurnaceType.int(subtype) if subtype.kind_of?(::Symbol) and type == :Furnace
            subtype = CivzoneType.int(subtype) if subtype.kind_of?(::Symbol) and type == :Civzone
            subtype = TrapType.int(subtype) if subtype.kind_of?(::Symbol) and type == :Trap
            bld.setSubtype(subtype)
            bld.setCustomType(custom)
            case type
            when :Furnace; bld.melt_remainder[world.raws.inorganics.length] = 0
            when :Coffin; bld.initBurialFlags
            when :Trap; bld.unk_cc = 500 if bld.trap_type == :PressurePlate
            when :Floodgate; bld.gate_flags.closed = true
            end
            bld
        end

        # used by building_setsize
        def building_check_bridge_support(bld)
            x1 = bld.x1-1
            x2 = bld.x2+1
            y1 = bld.y1-1
            y2 = bld.y2+1
            z = bld.z
            (x1..x2).each { |x|
                (y1..y2).each { |y|
                    next if ((x == x1 or x == x2) and
                             (y == y1 or y == y2))
                    if mb = map_block_at(x, y, z) and tile = mb.tiletype[x%16][y%16] and TiletypeShape::BasicShape[Tiletype::Shape[tile]] != :Open
                        bld.gate_flags.has_support = true
                        return
                    end
                }
            }
            bld.gate_flags.has_support = false
        end

        # sets x2/centerx/y2/centery from x1/y1/bldtype
        # x2/y2 preserved for :FarmPlot etc
        def building_setsize(bld)
            bld.x2 = bld.x1 if bld.x1 > bld.x2
            bld.y2 = bld.y1 if bld.y1 > bld.y2
            case bld.getType
            when :Bridge
                bld.centerx = bld.x1 + (bld.x2+1-bld.x1)/2
                bld.centery = bld.y1 + (bld.y2+1-bld.y1)/2
                building_check_bridge_support(bld)
            when :FarmPlot, :RoadDirt, :RoadPaved, :Stockpile, :Civzone
                bld.centerx = bld.x1 + (bld.x2+1-bld.x1)/2
                bld.centery = bld.y1 + (bld.y2+1-bld.y1)/2
            when :TradeDepot, :Shop
                bld.x2 = bld.x1+4
                bld.y2 = bld.y1+4
                bld.centerx = bld.x1+2
                bld.centery = bld.y1+2
            when :SiegeEngine, :Windmill, :Wagon
                bld.x2 = bld.x1+2
                bld.y2 = bld.y1+2
                bld.centerx = bld.x1+1
                bld.centery = bld.y1+1
            when :AxleHorizontal
                if bld.is_vertical == 1
                    bld.x2 = bld.centerx = bld.x1
                    bld.centery = bld.y1 + (bld.y2+1-bld.y1)/2
                else
                    bld.centerx = bld.x1 + (bld.x2+1-bld.x1)/2
                    bld.y2 = bld.centery = bld.y1
                end
            when :WaterWheel
                if bld.is_vertical == 1
                    bld.x2 = bld.centerx = bld.x1
                    bld.y2 = bld.y1+2
                    bld.centery = bld.y1+1
                else
                    bld.x2 = bld.x1+2
                    bld.centerx = bld.x1+1
                    bld.y2 = bld.centery = bld.y1
                end
            when :Workshop, :Furnace
                # Furnace = Custom or default case only
                case bld.type
                when :Quern, :Millstone, :Tool
                    bld.x2 = bld.centerx = bld.x1
                    bld.y2 = bld.centery = bld.y1
                when :Siege, :Kennels
                    bld.x2 = bld.x1+4
                    bld.y2 = bld.y1+4
                    bld.centerx = bld.x1+2
                    bld.centery = bld.y1+2
                when :Custom
                    if bdef = world.raws.buildings.all.binsearch(bld.getCustomType)
                        bld.x2 = bld.x1 + bdef.dim_x - 1
                        bld.y2 = bld.y1 + bdef.dim_y - 1
                        bld.centerx = bld.x1 + bdef.workloc_x
                        bld.centery = bld.y1 + bdef.workloc_y
                    end
                else
                    bld.x2 = bld.x1+2
                    bld.y2 = bld.y1+2
                    bld.centerx = bld.x1+1
                    bld.centery = bld.y1+1
                end
            when :ScrewPump
                case bld.direction
                when :FromEast
                    bld.x2 = bld.centerx = bld.x1+1
                    bld.y2 = bld.centery = bld.y1
                when :FromSouth
                    bld.x2 = bld.centerx = bld.x1
                    bld.y2 = bld.centery = bld.y1+1
                when :FromWest
                    bld.x2 = bld.x1+1
                    bld.y2 = bld.centery = bld.y1
                    bld.centerx = bld.x1
                else
                    bld.x2 = bld.x1+1
                    bld.y2 = bld.centery = bld.y1
                    bld.centerx = bld.x1
                end
            when :Well
                bld.bucket_z = bld.z
                bld.x2 = bld.centerx = bld.x1
                bld.y2 = bld.centery = bld.y1
            when :Construction
                bld.x2 = bld.centerx = bld.x1
                bld.y2 = bld.centery = bld.y1
                bld.setMaterialAmount(1)
                return
            else
                bld.x2 = bld.centerx = bld.x1
                bld.y2 = bld.centery = bld.y1
            end
            bld.setMaterialAmount((bld.x2-bld.x1+1)*(bld.y2-bld.y1+1)/4+1)
        end

        # set building at position, with optional width/height
        def building_position(bld, pos, w=nil, h=nil)
            if pos.respond_to?(:x1)
                x, y, z = pos.x1, pos.y1, pos.z
                w ||= pos.x2-pos.x1+1 if pos.respond_to?(:x2)
                h ||= pos.y2-pos.y1+1 if pos.respond_to?(:y2)
            elsif pos.respond_to?(:x)
                x, y, z = pos.x, pos.y, pos.z
            else
                x, y, z = pos
            end
            w ||= pos.w if pos.respond_to?(:w)
            h ||= pos.h if pos.respond_to?(:h)
            bld.x1 = x
            bld.y1 = y
            bld.z  = z
            bld.x2 = bld.x1+w-1 if w
            bld.y2 = bld.y1+h-1 if h
            building_setsize(bld)
        end

        # set map occupancy/stockpile/etc for a building
        def building_setoccupancy(bld)
            stockpile = (bld.getType == :Stockpile)
            complete = (bld.getBuildStage >= bld.getMaxBuildStage)
            extents = (bld.room.extents and bld.isExtentShaped)

            z = bld.z
            (bld.x1..bld.x2).each { |x|
                (bld.y1..bld.y2).each { |y|
                    next if extents and bld.room.extents[bld.room.width*(y-bld.room.y)+(x-bld.room.x)] == 0
                    next if not mb = map_block_at(x, y, z)
                    des = mb.designation[x%16][y%16]
                    des.pile = stockpile
                    des.dig = :No
                    if complete
                        bld.updateOccupancy(x, y)
                    else
                        mb.occupancy[x%16][y%16].building = :Planned
                    end
                }
            }
        end

        # link bld into other rooms if it is inside their extents or vice versa
        def building_linkrooms(bld)
            world.buildings.other[:IN_PLAY].each { |ob|
                next if ob.z != bld.z
                if bld.is_room and bld.room.extents
                    next if ob.is_room or ob.x1 < bld.room.x or ob.x1 >= bld.room.x+bld.room.width or ob.y1 < bld.room.y or ob.y1 >= bld.room.y+bld.room.height
                    next if bld.room.extents[bld.room.width*(ob.y1-bld.room.y)+(ob.x1-bld.room.x)] == 0
                    ui.equipment.update.buildings = true
                    bld.children << ob
                    ob.parents << bld
                elsif ob.is_room and ob.room.extents
                    next if bld.is_room or bld.x1 < ob.room.x or bld.x1 >= ob.room.x+ob.room.width or bld.y1 < ob.room.y or bld.y1 >= ob.room.y+ob.room.height
                    next if ob.room.extents[ob.room.width*(bld.y1-ob.room.y)+(bld.x1-ob.room.x)].to_i == 0
                    ui.equipment.update.buildings = true
                    ob.children << bld
                    bld.parents << ob
                end
            }
        end

        # link the building into the world, set map data, link rooms, bld.id
        def building_link(bld)
            bld.id = df.building_next_id
            df.building_next_id += 1

            world.buildings.all << bld
            bld.categorize(true)
            building_setoccupancy(bld) if bld.isSettingOccupancy
            building_linkrooms(bld)
        end

        # set a design for the building
        def building_createdesign(bld, rough=true)
            job = bld.jobs[0]
            job.mat_type = bld.mat_type
            job.mat_index = bld.mat_index
            if bld.needsDesign
                bld.design = BuildingDesign.cpp_new
                bld.design.flags.rough = rough
            end
        end

        # creates a job to build bld, return it
        def building_linkforconstruct(bld)
            building_link bld
            ref = GeneralRefBuildingHolderst.cpp_new
            ref.building_id = bld.id
            job = Job.cpp_new
            job.job_type = :ConstructBuilding
            job.pos = [bld.centerx, bld.centery, bld.z]
            job.general_refs << ref
            bld.jobs << job
            job_link job
            job
        end

        # construct a building with items or JobItems
        def building_construct(bld, items)
            job = building_linkforconstruct(bld)
            rough = false
            items.each { |item|
                if items.kind_of?(JobItem)
                    item.quantity = (bld.x2-bld.x1+1)*(bld.y2-bld.y1+1)/4+1 if item.quantity < 0
                    job.job_items << item
                else
                    job_attachitem(job, item, :Hauled)
                end
                rough = true if item.getType == :BOULDER
                bld.mat_type = item.getMaterial if bld.mat_type == -1
                bld.mat_index = item.getMaterialIndex if bld.mat_index == -1
            }
            building_createdesign(bld, rough)
        end

        # construct an abstract building (stockpile, farmplot, ...)
        def building_construct_abstract(bld)
            case bld.getType
            when :Stockpile
                max = df.world.buildings.other[:STOCKPILE].map { |s| s.stockpile_number }.max
                bld.stockpile_number = max.to_i + 1
            when :Civzone
                max = df.world.buildings.other[:ANY_ZONE].map { |z| z.zone_num }.max
                bld.zone_num = max.to_i + 1
            end
            building_link bld
            if !bld.flags.exists
                bld.flags.exists = true
                bld.initFarmSeasons
            end
        end

        def building_setowner(bld, unit)
            return unless bld.is_room
            return if bld.owner == unit
            
            if bld.owner
                if idx = bld.owner.owned_buildings.index { |ob| ob.id == bld.id }
                    bld.owner.owned_buildings.delete_at(idx)
                end
                if spouse = bld.owner.relations.spouse_tg and
                        idx = spouse.owned_buildings.index { |ob| ob.id == bld.id }
                    spouse.owned_buildings.delete_at(idx)
                end
            end
            bld.owner = unit
            if unit
                unit.owned_buildings << bld
                if spouse = bld.owner.relations.spouse_tg and
                        !spouse.owned_buildings.index { |ob| ob.id == bld.id } and
                        bld.canUseSpouseRoom
                    spouse.owned_buildings << bld
                end
            end
        end

        # creates a job to deconstruct the building
        def building_deconstruct(bld)
            job = Job.cpp_new
            refbuildingholder = GeneralRefBuildingHolderst.cpp_new
            job.job_type = :DestroyBuilding
            refbuildingholder.building_id = bld.id
            job.general_refs << refbuildingholder
            bld.jobs << job
            job_link job
            job
        end

        # exemple usage
        def buildbed(pos=cursor)
            raise 'where to ?' if pos.x < 0

            item = world.items.all.find { |i|
                i.kind_of?(ItemBedst) and
                item_isfree(i)
            }
            raise 'no free bed, build more !' if not item

            bld = building_alloc(:Bed)
            building_position(bld, pos)
            building_construct(bld, [item])
        end
    end
end
