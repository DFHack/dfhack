-- fixes some equipment corruption issues (bug 11014)

--[====[

fix/corrupt-equipment
=====================

Fixes some corruption that can occur in equipment lists, as in :bug:`11014`.

Note that there have been several possible types of corruption identified:

1. Items that have been deleted without being removed from the equipment lists
2. Items of the wrong type being stored in the equipment lists
3. Items of the wrong type being assigned to squad members

This script currently only fixes the first two, as they have been linked to the
majority of crashes.

Note that in some cases, multiple issues may be present, and may only be present
for a short window of time before DF crashes. To address this, running this
script with `repeat` is recommended. For example, to run this script every
100 ticks::

    repeat -name fix-corrupt-equipment -time 100 -timeUnits ticks -command [ fix/corrupt-equipment ]

To cancel it (which is likely safe if the script has not produced any output
in some time, and if you have saved first)::

    repeat -cancel fix-corrupt-equipment

Running this script with `repeat` on all saves is not recommended, as it can
have overhead (sometimes over 0.1 seconds on a large save). In general, running
this script with `repeat` is recommended if:

- You have experienced crashes likely caused by :bug:`11014`, and running this
  script a single time produces output but does not fix the crash
- You are running large military operations, or have sent squads out on raids

]====]

local utils = require("utils")

function fix_vector(vec, valid_items, valid_type, vector_desc)
  local raw_vec = df.reinterpret_cast("ptr-vector", vec)
  for i = #vec - 1, 0, -1 do
    if not valid_items[utils.addressof(raw_vec[i])] then
      print(string.format("Removing corrupted %s (index %i)", vector_desc, i))
      vec:erase(i)
    elseif valid_type and not valid_type:is_instance(vec[i]) then
      print(string.format("Removing %s of invalid type %s (index %i)",
        vector_desc, df.item_type[vec[i]:getType()], i))
      vec:erase(i)
    end
  end
end

function fix_equipment ()
  local valid_items = {}
  for _, item in ipairs(df.item.get_vector()) do
    valid_items[utils.addressof(item)] = true
  end

  local valid_types = {
    FLASK = df.item_flaskst,
    WEAPON = df.item_weaponst,
    ARMOR = df.item_armorst,
    SHOES = df.item_shoesst,
    SHIELD = df.item_shieldst,
    HELM = df.item_helmst,
    GLOVES = df.item_glovesst,
    AMMO = df.item_ammost,
    PANTS = df.item_pantsst,
    BACKPACK = df.item_backpackst,
    QUIVER = df.item_quiverst,
  }

  for category in pairs(df.global.plotinfo.equipment.items_unmanifested) do
    fix_vector(df.global.plotinfo.equipment.items_unmanifested[category], valid_items,
      valid_types[category], "unmanifested " .. category)
    fix_vector(df.global.plotinfo.equipment.items_unassigned[category], valid_items,
      valid_types[category], "unassigned " .. category)
    fix_vector(df.global.plotinfo.equipment.items_assigned[category], valid_items,
      valid_types[category], "assigned " .. category)
  end

  for i, squad in ipairs (df.global.world.squads.all) do
    if squad.entity_id == df.global.plotinfo.group_id then
      local squad_name = dfhack.TranslateName(squad.name, true)
      if squad.alias ~= "" then
        squad_name = squad.alias
      end
      squad_name = dfhack.df2utf(squad_name)

      for k, position in ipairs (squad.positions) do
        for l, item_id in ipairs (position.assigned_items) do
          local legal_type_found = false
          local item = df.item.find(item_id)

          -- the item itself may not be present - e.g. items belonging to military units on raids
          if item then
            local item_type_str = tostring(df.item_type[item:getType()])
            if not valid_types[item_type_str] then
              dfhack.printerr ("Item " .. tostring (l) .. " assigned to squad member " .. tostring (k) .. " of squad " .. squad_name ..
                               " is of unexpected type " .. item_type_str ..  ". Detection only. No action performed.")
            end
          end
        end
      end
    end
  end
end

fix_equipment ()
