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

        def map_tile_at(x, y=nil, z=nil)
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
                (v.tile_bitmask.bits[@dy] & (1 << @dx)) > 0
            }
        end

        # return the mat_index for the current tile (if in vein)
        def mat_index_vein
            v = vein
            v.inorganic_mat if v
        end

        # return the world_data.geo_biome for current tile
        def geo_biome
            b = designation.biome
            wd = df.world.world_data

            # region coords + [[-1, -1], [0, -1], ..., [1, 1]][b]
            # clipped to world dimensions
            rx = df.world.map.region_x/16
            rx -= 1 if b % 3 == 0 and rx > 0
            rx += 1 if b % 3 == 2 and rx < wd.world_width-1

            ry = df.world.map.region_y/16
            ry -= 1 if b < 3 and ry > 0
            ry += 1 if b > 5 and ry < wd.world_height-1

            wd.geo_biomes[ wd.region_map[rx][ry].geo_index ]
        end

        # return the world_data.geo_biome.layer for current tile
        def stone_layer
            geo_biome.layers[designation.geolayer_index]
        end

        # current tile mat_index (vein if applicable, or base material)
        def mat_index
            mat_index_vein or stone_layer.mat_index
        end

        # MaterialInfo: inorganic token for current tile
        def mat_info
            MaterialInfo.new(0, mat_index)
        end

        def inspect
            "#<MapTile pos=[#@x, #@y, #@z] shape=#{shape} tilemat=#{tilemat} material=#{mat_info.token}>"
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
    end
end
