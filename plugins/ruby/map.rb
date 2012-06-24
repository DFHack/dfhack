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
end
