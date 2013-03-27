module DFHack
    class << self
        # return a Plant
        # arg similar to unit.rb/unit_find, no menu
        def plant_find(what=cursor)
            if what.kind_of?(Integer)
                world.items.all.binsearch(what)
            elsif what.respond_to?(:x) or what.respond_to?(:pos)
                world.plants.all.find { |p| same_pos?(what, p) }
            else
                raise "what what?"
            end
        end

        def each_tree(material=:any)
            @raws_tree_name ||= {}
            if @raws_tree_name.empty?
                df.world.raws.plants.all.each_with_index { |p, idx|
                    @raws_tree_name[idx] = p.id if p.flags[:TREE]
                }
            end

            if material != :any
                mat = match_rawname(material, @raws_tree_name.values)
                unless wantmat = @raws_tree_name.index(mat)
                    raise "invalid tree material #{material}"
                end
            end

            world.plants.all.each { |plant|
                next if not @raws_tree_name[plant.material]
                next if wantmat and plant.material != wantmat
                yield plant
            }
        end

        def each_shrub(material=:any)
            @raws_shrub_name ||= {}
            if @raws_tree_name.empty?
                df.world.raws.plants.all.each_with_index { |p, idx|
                    @raws_shrub_name[idx] = p.id if not p.flags[:GRASS] and not p.flags[:TREE]
                }
            end

            if material != :any
                mat = match_rawname(material, @raws_shrub_name.values)
                unless wantmat = @raws_shrub_name.index(mat)
                    raise "invalid shrub material #{material}"
                end
            end
        end

        SaplingToTreeAge = 120960
        def cuttrees(material=nil, count_max=100, quiet=false)
            if !material
                # list trees
                cnt = Hash.new(0)
                each_tree { |plant|
                    next if plant.grow_counter < SaplingToTreeAge
                    next if map_designation_at(plant).hidden
                    cnt[plant.material] += 1
                }
                cnt.sort_by { |mat, c| c }.each { |mat, c|
                    name = @raws_tree_name[mat]
                    puts " #{name} #{c}" unless quiet
                }
            else
                cnt = 0
                each_tree(material) { |plant|
                    next if plant.grow_counter < SaplingToTreeAge
                    b = map_block_at(plant)
                    d = b.designation[plant.pos.x%16][plant.pos.y%16]
                    next if d.hidden
                    if d.dig == :No
                        d.dig = :Default
                        b.flags.designated = true
                        cnt += 1
                        break if cnt == count_max
                    end
                }
                puts "Updated #{cnt} plant designations" unless quiet
            end
        end

        def growtrees(material=nil, count_max=100, quiet=false)
            if !material
                # list plants
                cnt = Hash.new(0)
                each_tree { |plant|
                    next if plant.grow_counter >= SaplingToTreeAge
                    next if map_designation_at(plant).hidden
                    cnt[plant.material] += 1
                }
                cnt.sort_by { |mat, c| c }.each { |mat, c|
                    name = @raws_tree_name[mat]
                    puts " #{name} #{c}" unless quiet
                }
            else
                cnt = 0
                each_tree(material) { |plant|
                    next if plant.grow_counter >= SaplingToTreeAge
                    next if map_designation_at(plant).hidden
                    plant.grow_counter = SaplingToTreeAge
                    cnt += 1
                    break if cnt == count_max
                }
                puts "Grown #{cnt} saplings" unless quiet
            end
        end
    end
end
