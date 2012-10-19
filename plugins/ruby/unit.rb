module DFHack
    class << self
        # return an Unit
        # with no arg, return currently selected unit in df UI ('v' or 'k' menu)
        # with numeric arg, search unit by unit.id
        # with an argument that respond to x/y/z (eg cursor), find first unit at this position
        def unit_find(what=:selected, y=nil, z=nil)
            if what == :selected
                case curview._rtti_classname
                when :viewscreen_itemst
                    ref = curview.entry_ref[curview.cursor_pos]
                    ref.unit_tg if ref.kind_of?(GeneralRefUnit)
                when :viewscreen_unitlistst
                    v = curview
                    v.units[v.page][v.cursor_pos[v.page]]
                when :viewscreen_petst
                    v = curview
                    case v.mode
                    when :List
                        v.animal[v.cursor].unit if !v.is_vermin[v.cursor]
                    when :SelectTrainer
                        v.trainer_unit[v.trainer_cursor]
                    end
                else
                    case ui.main.mode
                    when :ViewUnits
                        # nobody selected => idx == 0
                        v = world.units.active[ui_selected_unit]
                        v if v and v.pos.z == cursor.z
                    when :LookAround
                        k = ui_look_list.items[ui_look_cursor]
                        k.unit if k.type == :Unit
                    else
                        ui.follow_unit_tg if ui.follow_unit != -1
                    end
                end
            elsif what.kind_of?(Integer)
                # search by id
                return world.units.all.binsearch(what) if not z
                # search by coords
                x = what
                world.units.all.find { |u| u.pos.x == x and u.pos.y == y and u.pos.z == z }
            elsif what.respond_to?(:x) or what.respond_to?(:pos)
                world.units.all.find { |u| same_pos?(what, u) }
            else
                raise "what what?"
            end
        end

        # returns an Array of all units that are current fort citizen (dwarves, on map, not hostile)
        def unit_citizens
            world.units.active.find_all { |u| 
                unit_iscitizen(u)
            }
        end

        def unit_iscitizen(u)
            u.race == ui.race_id and u.civ_id == ui.civ_id and !u.flags1.dead and !u.flags1.merchant and !u.flags1.forest and
            !u.flags1.diplomat and !u.flags2.resident and !u.flags3.ghostly and
            !u.curse.add_tags1.OPPOSED_TO_LIFE and !u.curse.add_tags1.CRAZED and
            u.mood != :Berserk
            # TODO check curse ; currently this should keep vampires, but may include werebeasts
        end

        # list workers (citizen, not crazy / child / inmood / noble)
        def unit_workers
            world.units.active.find_all { |u|
                unit_isworker(u)
            }
        end

        def unit_isworker(u)
            unit_iscitizen(u) and
            u.mood == :None and
            u.profession != :CHILD and
            u.profession != :BABY and
            # TODO MENIAL_WORK_EXEMPTION_SPOUSE
            !unit_entitypositions(u).find { |pos| pos.flags[:MENIAL_WORK_EXEMPTION] }
        end

        # list currently idle workers
        def unit_idlers
            world.units.active.find_all { |u|
                unit_isidler(u)
            }
        end

        def unit_isidler(u)
            unit_isworker(u) and
            # current_job includes eat/drink/sleep/pickupequip
            !u.job.current_job and 
            # filter 'attend meeting'
            not u.specific_refs.find { |s| s.type == :ACTIVITY } and
            # filter soldiers (TODO check schedule)
            u.military.squad_index == -1 and
            # filter 'on break'
            not u.status.misc_traits.find { |t| t.id == :OnBreak }
        end

        def unit_entitypositions(unit)
            list = []
            return list if not hf = unit.hist_figure_tg
            hf.entity_links.each { |el|
                next if el._rtti_classname != :histfig_entity_link_positionst
                next if not ent = el.entity_tg
                next if not pa = ent.positions.assignments.binsearch(el.assignment_id)
                next if not pos = ent.positions.own.binsearch(pa.position_id)
                list << pos
            }
            list
        end
    end

    class LanguageName
        def to_s(english=false)
            df.translate_name(self, english)
        end
    end
end
