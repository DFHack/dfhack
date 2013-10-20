module DFHack
    class << self
        # return a map block by tile coordinates
        # you can also use find_map_block(cursor) or anything that respond to x/y/z
        def map_block_at(x, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)
            if x >= 0 and x < world.map.x_count and y >= 0 and y < world.map.y_count and z >= 0 and z < world.map.z_count
                world.map.block_index[x/16][y/16][z]
            end
        end

        def map_designation_at(x, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)
            if b = map_block_at(x, y, z)
                b.designation[x%16][y%16]
            end
        end

        def map_occupancy_at(x, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)
            if b = map_block_at(x, y, z)
                b.occupancy[x%16][y%16]
            end
        end

        def map_tile_at(x=df.cursor, y=nil, z=nil)
            x = x.pos if x.respond_to?(:pos)
            x, y, z = x.x, x.y, x.z if x.respond_to?(:x)
            b = map_block_at(x, y, z)
            MapTile.new(b, x, y, z) if b
        end

        # yields every map block
        def each_map_block
            (0...world.map.x_count_block).each { |xb|
                xl = world.map.block_index[xb]
                (0...world.map.y_count_block).each { |yb|
                    yl = xl[yb]
                    (0...world.map.z_count_block).each { |z|
                        p = yl[z]
                        yield p if p
                    }
                }
            }
        end

        # yields every map block for a given z level
        def each_map_block_z(z)
            (0...world.map.x_count_block).each { |xb|
                xl = world.map.block_index[xb]
                (0...world.map.y_count_block).each { |yb|
                    p = xl[yb][z]
                    yield p if p
                }
            }
        end
    end

    class MapTile
        attr_accessor :x, :y, :z, :dx, :dy, :mapblock
        def initialize(b, x, y, z)
            @x, @y, @z = x, y, z
            @dx, @dy = @x&15, @y&15
            @mapblock = b
        end

        def offset(dx, dy=nil, dz=0)
            if dx.respond_to?(:x)
                dz = dx.z if dx.respond_to?(:z)
                dx, dy = dx.x, dx.y
            end
            df.map_tile_at(@x+dx, @y+dy, @z+dz)
        end

        def designation
            @mapblock.designation[@dx][@dy]
        end

        def occupancy
            @mapblock.occupancy[@dx][@dy]
        end

        def tiletype
            @mapblock.tiletype[@dx][@dy]
        end

        def tiletype=(t)
            @mapblock.tiletype[@dx][@dy] = t
        end

        def caption
            Tiletype::Caption[tiletype]
        end

        def shape
            Tiletype::Shape[tiletype]
        end

        def tilemat
            Tiletype::Material[tiletype]
        end

        def variant
            Tiletype::Variant[tiletype]
        end

        def special
            Tiletype::Special[tiletype]
        end

        def direction
            Tiletype::Direction[tiletype]
        end

        def shape_caption
            TiletypeShape::Caption[shape]
        end

        def shape_basic
            TiletypeShape::BasicShape[shape]
        end

        def shape_passablelow
            TiletypeShape::PassableLow[shape]
        end

        def shape_passablehigh
            TiletypeShape::PassableHigh[shape]
        end

        def shape_passableflow
            TiletypeShape::PassableFlow[shape]
        end

        def shape_walkable
            TiletypeShape::Walkable[shape]
        end


        # return all veins for current mapblock
        def all_veins
            mapblock.block_events.grep(BlockSquareEventMineralst)
        end

        # return the vein applicable to current tile
        def vein
            # last vein wins
            all_veins.reverse.find { |v|
                v.tile_bitmask.bits[@dy][@dx] > 0
            }
        end

        # return the first BlockBurrow this tile is in (nil if none)
        def burrow
            mapblock.block_burrows.find { |b|
                b.tile_bitmask.bits[@dy][@dx] > 0
            }
        end

        # return the array of BlockBurrow this tile is in
        def all_burrows
            mapblock.block_burrows.find_all { |b|
                b.tile_bitmask.bits[@dy][@dx] > 0
            }
        end

        # return the mat_index for the current tile (if in vein)
        def mat_index_vein
            v = vein
            v.inorganic_mat if v
        end

        # return the RegionMapEntry (from designation.biome)
        def region_map_entry
            b = mapblock.region_offset[designation.biome]
            wd = df.world.world_data

            # region coords + [[-1, -1], [0, -1], ..., [1, 1]][b]
            # clipped to world dimensions
            rx = df.world.map.region_x/16
            rx -= 1 if b % 3 == 0 and rx > 0
            rx += 1 if b % 3 == 2 and rx < wd.world_width-1

            ry = df.world.map.region_y/16
            ry -= 1 if b < 3 and ry > 0
            ry += 1 if b > 5 and ry < wd.world_height-1

            wd.region_map[rx][ry]
        end

        # return the world_data.geo_biome for current tile
        def geo_biome
            df.world.world_data.geo_biomes[ region_map_entry.geo_index ]
        end

        # return the world_data.geo_biome.layer for current tile
        def stone_layer
            geo_biome.layers[designation.geolayer_index]
        end

        # MaterialInfo: token for current tile, based on tilemat (vein, soil, plant, lava_stone...)
        def mat_info
            case tilemat
            when :SOIL
                base = stone_layer
                if !df.world.raws.inorganics[base.mat_index].flags[:SOIL_ANY]
                    base = geo_biome.layers.find_all { |l| df.world.raws.inorganics[l.mat_index].flags[:SOIL_ANY] }.last
                end
                mat_index = (base ? base.mat_index : rand(df.world.raws.inorganics.length))
                MaterialInfo.new(0, mat_index)

            when :STONE
                base = stone_layer
                if df.world.raws.inorganics[base.mat_index].flags[:SOIL_ANY]
                    base = geo_biome.layers.find { |l| !df.world.raws.inorganics[l.mat_index].flags[:SOIL_ANY] }
                end
                mat_index = (base ? base.mat_index : rand(df.world.raws.inorganics.length))
                MaterialInfo.new(0, mat_index)

            when :MINERAL
                mat_index = (mat_index_vein || stone_layer.mat_index)
                MaterialInfo.new(0, mat_index)

            when :LAVA_STONE
                # XXX this is wrong
                # maybe should search world.region_details.pos == biome_region_pos ?
                idx = mapblock.region_offset[designation.biome]
                mat_index = df.world.world_data.region_details[idx].lava_stone
                MaterialInfo.new(0, mat_index)

            when :FEATURE
                if designation.feature_local
                    mx = mapblock.region_pos.x
                    my = mapblock.region_pos.y
                    df.decode_mat(df.world.world_data.feature_map[mx/16][my/16].features.feature_init[mx%16][my%16][mapblock.local_feature])
                elsif designation.feature_global
                    df.decode_mat(df.world.world_data.underground_regions[mapblock.global_feature].feature_init)
                else
                    MaterialInfo.new(-1, -1)
                end

            when :FROZEN_LIQUID
                MaterialInfo.new('WATER')

            # TODO
            #when :PLANT
            #when :GRASS_DARK, :GRASS_DEAD, :GRASS_DRY, :GRASS_LIGHT
            #when :CONSTRUCTION
            else    # AIR ASHES BROOK CAMPFIRE DRIFTWOOD FIRE HFS MAGMA POOL RIVER
                MaterialInfo.new(-1, -1)
            end
        end

        def mat_type
            mat_info.mat_type
        end

        def mat_index
            mat_info.mat_index
        end

        def inspect
            "#<MapTile pos=[#@x, #@y, #@z] shape=#{shape} tilemat=#{tilemat} material=#{mat_info.token}>"
        end

        def dig(mode=:Default)
            if mode == :Smooth
                if (tilemat == :STONE or tilemat == :MINERAL) and caption !~ /smooth|pillar|fortification/i and   # XXX caption..
                    designation.smooth == 0 and (designation.hidden or not df.world.job_list.find { |j|
                        # the game removes 'smooth' designation as soon as it assigns a job, if we
                        # re-set it the game may queue another :DetailWall that will carve a fortification
                        (j.job_type == :DetailWall or j.job_type == :DetailFloor) and df.same_pos?(j, self)
                    })
                    designation.dig = :No
                    designation.smooth = 1
                    mapblock.flags.designated = true
                end
            else
                return if mode != :No and designation.dig == :No and not designation.hidden and df.world.job_list.find { |j|
                    # someone already enroute to dig here, avoid 'Inappropriate dig square' spam
                    JobType::Type[j.job_type] == :Digging and df.same_pos?(j, self)
                }
                designation.dig = mode
                mapblock.flags.designated = true if mode != :No
            end
        end

        def spawn_liquid(quantity, is_magma=false, flowing=true)
            designation.flow_size = quantity
            designation.liquid_type = (is_magma ? :Magma : :Water)
            designation.flow_forbid = true if is_magma or quantity >= 4

            if flowing
                mapblock.flags.update_liquid = true
                mapblock.flags.update_liquid_twice = true

                zf = df.world.map.z_level_flags[z]
                zf.update = true
                zf.update_twice = true
            end
        end

        def spawn_water(quantity=7)
            spawn_liquid(quantity)
        end

        def spawn_magma(quantity=7)
            spawn_liquid(quantity, true)
        end

        # yield a serie of tiles until the block returns true, returns the matching tile
        # the yielded tiles form a (squared) spiral centered here in the current zlevel
        # eg for radius 4, yields (-4, -4), (-4, -3), .., (-4, 3),
        #   (-4, 4), (-3, 4), .., (4, 4), .., (4, -4), .., (-3, -4)
        # then move on to radius 5
        def spiral_search(maxradius=100, minradius=0, step=1)
            if minradius == 0
                    return self if yield self
                    minradius += step
            end

            sides = [[0, 1], [1, 0], [0, -1], [-1, 0]]
            (minradius..maxradius).step(step) { |r|
                sides.length.times { |s|
                    dxr, dyr = sides[(s-1) % sides.length]
                    dx, dy = sides[s]
                    (-r...r).step(step) { |v|
                        t = offset(dxr*r + dx*v, dyr*r + dy*v)
                        return t if t and yield t
                    }
                }
            }
            nil
        end

        # returns dx^2+dy^2+dz^2
        def distance_to(ot)
            (x-ot.x)**2 + (y-ot.y)**2 + (z-ot.z)**2
        end
    end
end
