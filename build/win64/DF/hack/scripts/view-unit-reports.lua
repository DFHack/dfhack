-- View combat reports for currently selected unit
--[====[

view-unit-reports
=================
Show combat reports for the current unit.

Current unit is unit near cursor in 'v', selected in 'u' list,
unit/corpse/splatter at cursor in 'k'. And newest unit with race when
'k' at race-specific blood spatter.

]====]

local function get_combat_logs(unit)
   local output = {}
   for _,report_id in pairs(unit.reports.log.Combat) do
      local report = df.report.find(report_id)
      if report then
         output[#output + 1] = report
      end
   end
   return output
end

local function view_unit_report(unit)
   local report_view = df.viewscreen_announcelistst:new()
   report_view.unit = unit

   for _,report in pairs(get_combat_logs(unit)) do
      report_view.reports:insert('#', report)
   end

   report_view.sel_idx = #report_view.reports - 1

   dfhack.screen.show(report_view)
end

local function find_last_unit_with_combat_log_and_race(race_id)
   local output = nil
   for _, unit in pairs(df.global.world.units.all) do
      if unit.race == race_id and #get_combat_logs(unit) >= 1 then
         output = unit
      end
   end
   return output
end

local function current_selected_unit()
   -- 'v' view, 'k' at unit, unit list view
   local unit = dfhack.gui.getSelectedUnit(true)
   if unit then
      return unit
   end

   -- 'k' over a corpse
   local item = dfhack.gui.getSelectedItem(true) --as:df.item_corpsest
   if df.item_corpsest:is_instance(item) then
      return df.unit.find(item.unit_id)
   end

   -- 'k' over some spatter (e.g. blood)
   local screen_string = dfhack.gui.getFocusString(dfhack.gui.getCurViewscreen())
   if screen_string == "dwarfmode/LookAround/Spatter" then
      local look_at = df.global.ui_look_list.items[df.global.ui_look_cursor]
      local matinfo = dfhack.matinfo.decode(look_at.spatter_mat_type, look_at.spatter_mat_index)

      -- spatter is related to historical figure
      if matinfo.figure then
         return df.unit.find(matinfo.figure.unit_id)
      end

      -- spatter is only related to a race. find good unit with that race.
      if matinfo.creature then
         return find_last_unit_with_combat_log_and_race(matinfo.index)
      end
   end

   return nil
end

local unit = current_selected_unit()
if unit then
   view_unit_report(unit)
end
