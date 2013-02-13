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
                when :viewscreen_dwarfmodest
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
                when :viewscreen_dungeonmodest
                    case ui_advmode.menu
                    when :Default
                        world.units.active[0]
                    else
                        unit_find(cursor)   # XXX
                    end
                when :viewscreen_dungeon_monsterstatusst
                    curview.unit
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

        def unit_testflagcurse(u, flag)
            return false if u.curse.rem_tags1.send(flag)
            return true if u.curse.add_tags1.send(flag)
            return false if u.caste < 0
            u.race_tg.caste[u.caste].flags[flag]
        end

        def unit_isfortmember(u)
            # RE from viewscreen_unitlistst ctor
            return false if df.gamemode != :DWARF or
                    u.mood == :Berserk or
                    unit_testflagcurse(u, :CRAZED) or
                    unit_testflagcurse(u, :OPPOSED_TO_LIFE) or
                    u.enemy.undead or
                    u.flags3.ghostly or
                    u.flags1.marauder or u.flags1.active_invader or u.flags1.invader_origin or
                    u.flags1.forest or
                    u.flags1.merchant or u.flags1.diplomat
            return true if u.flags1.tame
            return false if u.flags2.underworld or u.flags2.resident or
                    u.flags2.visitor_uninvited or u.flags2.visitor or
                    u.civ_id == -1 or
                    u.civ_id != df.ui.civ_id
            true
        end

        # return the page in viewscreen_unitlist where the unit would appear
        def unit_category(u)
            return if u.flags1.left or u.flags1.incoming
            # return if hostile & unit_invisible(u) (hidden_in_ambush or caged+mapblock.hidden or caged+holder.ambush
            return :Dead if u.flags1.dead
            return :Dead if u.flags3.ghostly # hostile ?
            return :Others if !unit_isfortmember(u)
            casteflags = u.race_tg.caste[u.caste].flags if u.caste >= 0
            return :Livestock if casteflags and (casteflags[:PET] or casteflags[:PET_EXOTIC])
            return :Citizens if unit_testflagcurse(u, :CAN_SPEAK)
            :Livestock
            # some other stuff with ui.race_id ? (jobs only?)
        end

        # merchant: df.ui.caravans.find { |cv| cv.entity == u.civ_id }
        # diplomat: df.ui.dip_meeting_info.find { |m| m.diplomat_id == u.hist_figure_id or m.diplomat_id2 == u.hist_figure_id }


        def unit_nemesis(u)
            if ref = u.general_refs.find { |r| r.kind_of?(DFHack::GeneralRefIsNemesisst) }
                ref.nemesis_tg
            end
        end

        # return the subcategory for :Others (from vs_unitlist)
        def unit_other_category(u)
            # comment is actual code returned by the df function
            return :Berserk if u.mood == :Berserk  # 5
            return :Berserk if unit_testflagcurse(u, :CRAZED)    # 14
            return :Undead if unit_testflagcurse(u, :OPPOSED_TO_LIFE)   # 1
            return :Undead if u.flags3.ghostly  # 15

            if df.gamemode == :ADVENTURE
                return :Hostile if u.civ_id == -1   # 2
                if u.animal.population.region_x == -1
                    return :Wild if u.flags2.roaming_wilderness_population_source_not_a_map_feature # 0
                else
                    return :Hostile if u.flags2.important_historical_figure and n = unit_nemesis(u) and n.flags[:ACTIVE_ADVENTURER] # 2
                end
                return :Hostile if u.flags2.resident    # 3
                return :Hostile # 4
            end

            return :Invader if u.flags1.active_invader or u.flags1.invader_origin   # 6
            return :Friendly if u.flags1.forest or u.flags1.merchant or u.flags1.diplomat   # 8
            return :Hostile if u.flags1.tame    # 7

            if u.civ_id != -1
                return :Unsure if u.civ_id != df.ui.civ_id or u.flags1.resident or u.flags1.visitor or u.flags1.visitor_uninvited   # 10
                return :Hostile # 7

            elsif u.animal.population.region_x == -1
                return :Friendly if u.flags2.visitor    # 8
                return :Uninvited if u.flags2.visitor_uninvited # 12
                return :Underworld if r = u.race_tg and r.underground_layer_min == 5    # 9
                return :Resident if u.flags2.resident   # 13
                return :Friendly    # 8

            else
                return :Friendly if u.flags2.visitor    # 8
                return :Underworld if r = u.race_tg and r.underground_layer_min == 5    # 9
                return :Wild if u.animal.population.feature_idx == -1 and u.animal.population.cave_id == -1 # 0
                return :Wild    # 11
            end
        end

        def unit_iscitizen(u)
            unit_category(u) == :Citizens
        end

        def unit_hostiles
            world.units.active.find_all { |u|
                unit_ishostile(u)
            }
        end

        # returns if an unit is openly hostile
        # does not include ghosts / wildlife
        def unit_ishostile(u)
            # return true if u.flags3.ghostly and not u.flags1.dead
            return unless unit_category(u) == :Others

            case unit_other_category(u)
            when :Berserk, :Undead, :Hostile, :Invader, :Underworld
                # XXX :Resident, :Uninvited?
                true

            when :Unsure
                # from df code, with removed duplicate checks already in other_category
                return true if u.enemy.undead or u.flags3.ghostly or u.flags1.marauder 
                return false if u.flags1.forest or u.flags1.merchant or u.flags1.diplomat or u.flags2.visitor
                return true if u.flags1.tame or u.flags2.underworld

                if histfig = u.hist_figure_tg
                    group = df.ui.group_tg
                    case unit_checkdiplomacy_hf_ent(histfig, group)
                    when 4, 5
                        true
                    end

                elsif diplo = u.civ_tg.unknown1b.diplomacy.binsearch(df.ui.group_id, :group_id)
                    diplo.relation != 1 and diplo.relation != 5

                else
                    u.animal.population.region_x != -1 or u.flags2.resident or u.flags2.visitor_uninvited
                end
            end
        end

        def unit_checkdiplomacy_hf_ent(histfig, group)
            var_3d = var_3e = var_45 = var_46 = var_47 = var_48 = var_49 = nil

            var_3d = 1 if group.type == :Outcast or group.type == :NomadicGroup or
            (group.type == :Civilization and group.entity_raw.flags[:LOCAL_BANDITRY])

            histfig.entity_links.each { |link|
                if link.entity_id == group.id
                    case link.getType
                    when :MEMBER, :MERCENARY, :SLAVE, :PRISONER, :POSITION, :HERO
                        var_47 = 1
                    when :FORMER_MEMBER, :FORMER_MERCENARY, :FORMER_SLAVE, :FORMER_PRISONER
                        var_48 = 1
                    when :ENEMY
                        var_49 = 1
                    when :CRIMINAL
                        var_45 = 1
                    end
                else
                    case link.getType
                    when :MEMBER, :MERCENARY, :SLAVE
                        if link_entity = link.entity_tg
                            diplo = group.unknown1b.diplomacy.binsearch(link.entity_id, :group_id)
                            case diplo.relation
                            when 0, 3, 4
                                var_48 = 1
                            when 1, 5
                                var_46 = 1
                            end

                            var_3e = 1 if link_entity.type == :Outcast or link_entity.type == :NomadicGroup or
                            (link_entity.type == :Civilization and link_entity.entity_raw.flags[:LOCAL_BANDITRY])
                        end
                    end
                end
            }

            if var_49
                4
            elsif var_46
                5
            elsif !var_47 and group.resources.ethic[:KILL_NEUTRAL] == 16
                4
            elsif df.gamemode == :ADVENTURE and !var_47 and (var_3e or !var_3d)
                4
            elsif var_45
                3
            elsif var_47
                2
            elsif var_48
                1
            else
                0
            end
        end


        # list workers (citizen, not crazy / child / inmood / noble)
        def unit_workers
            world.units.active.find_all { |u|
                unit_isworker(u)
            }
        end

        def unit_isworker(u)
            unit_iscitizen(u) and
            u.race == df.ui.race_id and
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
            u.military.squad_id == -1 and
            # filter 'on break'
            not u.status.misc_traits.find { |t| t.id == :OnBreak }
        end

        def unit_entitypositions(unit)
            list = []
            return list if not histfig = unit.hist_figure_tg
            histfig.entity_links.each { |el|
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
