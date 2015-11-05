-- unit-info-viewer.lua
-- Displays age, birth, maxage, shearing, milking, grazing, egg laying, body size, and death info about a unit. Recommended keybinding Alt-I
-- version 1.04
-- original author: Kurik Amudnil
-- edited by expwnent
--[[=begin

gui/unit-info-viewer
====================
Displays age, birth, maxage, shearing, milking, grazing, egg laying, body size,
and death info about a unit. Recommended keybinding :kbd:`Alt`:kbd:`I`.

=end]]
local gui = require 'gui'
local widgets =require 'gui.widgets'
local utils = require 'utils'

local DEBUG = false
if DEBUG then print('-----') end

local pens = {
 BLACK = dfhack.pen.parse{fg=COLOR_BLACK,bg=0},
 BLUE = dfhack.pen.parse{fg=COLOR_BLUE,bg=0},
 GREEN = dfhack.pen.parse{fg=COLOR_GREEN,bg=0},
 CYAN = dfhack.pen.parse{fg=COLOR_CYAN,bg=0},
 RED = dfhack.pen.parse{fg=COLOR_RED,bg=0},
 MAGENTA = dfhack.pen.parse{fg=COLOR_MAGENTA,bg=0},
 BROWN = dfhack.pen.parse{fg=COLOR_BROWN,bg=0},
 GREY = dfhack.pen.parse{fg=COLOR_GREY,bg=0},
 DARKGREY = dfhack.pen.parse{fg=COLOR_DARKGREY,bg=0},
 LIGHTBLUE = dfhack.pen.parse{fg=COLOR_LIGHTBLUE,bg=0},
 LIGHTGREEN = dfhack.pen.parse{fg=COLOR_LIGHTGREEN,bg=0},
 LIGHTCYAN = dfhack.pen.parse{fg=COLOR_LIGHTCYAN,bg=0},
 LIGHTRED = dfhack.pen.parse{fg=COLOR_LIGHTRED,bg=0},
 LIGHTMAGENTA = dfhack.pen.parse{fg=COLOR_LIGHTMAGENTA,bg=0},
 YELLOW = dfhack.pen.parse{fg=COLOR_YELLOW,bg=0},
 WHITE = dfhack.pen.parse{fg=COLOR_WHITE,bg=0},
}

function getUnit_byID(id) -- get unit by id from units.all via binsearch
 if type(id) == 'number' then
  -- (vector,key,field,cmpfun,min,max) { item/nil , found true/false , idx/insert at }
  return utils.binsearch(df.global.world.units.all,id,'id')
 end
end

function getUnit_byVS(silent)    -- by view screen mode
 silent = silent or false
 -- if not world loaded, return nil ?
 local u,tmp -- u: the unit to return ; tmp: temporary for intermediate tests/return values
 local v = dfhack.gui.getCurViewscreen()
 u = dfhack.gui.getSelectedUnit(true) -- supports gui scripts/plugin that provide a hook for getSelectedUnit()
 if u then
  return u
 -- else: contexts not currently supported by dfhack.gui.getSelectedUnit()
 elseif df.viewscreen_dwarfmodest:is_instance(v) then
  tmp = df.global.ui.main.mode
  if tmp == 17 or tmp == 42 or tmp == 43 then
   -- context: @dwarfmode/QueryBuiding/Some/Cage    -- (q)uery cage
   -- context: @dwarfmode/ZonesPenInfo/AssignUnit    -- i (zone) -> pe(N)
   -- context: @dwarfmode/ZonesPitInfo                -- i (zone) -> (P)it
   u = df.global.ui_building_assign_units[df.global.ui_building_item_cursor]
  elseif tmp == 49 and df.global.ui.burrows.in_add_units_mode then
   -- @dwarfmode/Burrows/AddUnits
   u = df.global.ui.burrows.list_units[ df.global.ui.burrows.unit_cursor_pos ]

  elseif df.global.ui.follow_unit ~= -1 then
   -- context: follow unit mode
   u = getUnit_byID(df.global.ui.follow_unit)
  end -- end viewscreen_dwarfmodest
 elseif df.viewscreen_petst:is_instance(v) then
  -- context: @pet/List/Unit -- z (status) -> animals
  if v.mode == 0 then -- List
   if not v.is_vermin[v.cursor] then
    u = v.animal[v.cursor].unit
   end
  --elseif v.mode = 1 then -- training knowledge (no unit reference)
  elseif v.mode == 2 then -- select trainer
   u = v.trainer_unit[v.trainer_cursor]
 end
 elseif df.viewscreen_layer_workshop_profilest:is_instance(v) then
  -- context: @layer_workshop_profile/Unit     -- (q)uery workshop -> (P)rofile -- df.global.ui.main.mode == 17
  u = v.workers[v.layer_objects[0].cursor]
 elseif df.viewscreen_layer_overall_healthst:is_instance(v) then
  -- context @layer_overall_health/Units -- z -> health
  u = v.unit[v.layer_objects[0].cursor]
 elseif df.viewscreen_layer_militaryst:is_instance(v) then
  local PG_ASSIGNMENTS = 0
  local PG_EQUIPMENT = 2
  local TB_POSITIONS = 1
  local TB_CANDIDATES = 2
  -- layer_objects[0: squads list; 1: positions list; 2: candidates list]
  -- page  0:positions/assignments  1:alerts  2:equipment  3:uniforms  4:supplies  5:ammunition
  if v.page == PG_ASSIGNMENTS and v.layer_objects[TB_CANDIDATES].enabled and v.layer_objects[TB_CANDIDATES].active then
   -- context: @layer_military/Positions/Position/Candidates    -- m -> Candidates
   u = v.positions.candidates[v.layer_objects[TB_CANDIDATES].cursor]
  elseif v.page == PG_ASSIGNMENTS and v.layer_objects[TB_POSITIONS].enabled and v.layer_objects[TB_POSITIONS].active then
   -- context: @layer_military/Positions/Position    -- m -> Positions
   u = v.positions.assigned[v.layer_objects[TB_POSITIONS].cursor]
  elseif v.page == PG_EQUIPMENT and v.layer_objects[TB_POSITIONS].enabled and v.layer_objects[TB_POSITIONS].active then
   -- context: @layer_military/Equip/Customize/View/Position    -- m -> (e)quip -> Positions
   -- context: @layer_military/Equip/Uniform/Positions    -- m -> (e)quip -> assign (U)niforms -> Positions
   u = v.equip.units[v.layer_objects[TB_POSITIONS].cursor]
  end
 elseif df.viewscreen_layer_noblelistst:is_instance(v) then
  if v.mode == 0 then
   -- context: @layer_noblelist/List    -- (n)obles
   u = v.info[v.layer_objects[v.mode].cursor].unit
  elseif v.mode == 1 then
   -- context: @layer_noblelist/Appoint    -- (n)obles -> (r)eplace
   u = v.candidates[v.layer_objects[v.mode].cursor].unit
  end
 elseif df.viewscreen_unitst:is_instance(v) then
  -- @unit    -- (v)unit -> z ; loo(k) -> enter ; (n)obles -> enter ; others
  u = v.unit
 elseif df.viewscreen_customize_unitst:is_instance(v) then
  -- @customize_unit    -- @unit -> y
  u = v.unit
 elseif df.viewscreen_layer_unit_healthst:is_instance(v) then
  -- @layer_unit_health    -- @unit -> h ; @layer_overall_health/Units -> enter
  if df.viewscreen_layer_overall_healthst:is_instance(v.parent) then
   -- context @layer_overall_health/Units     -- z (status)-> health
   u = v.parent.unit[v.parent.layer_objects[0].cursor]
  elseif df.viewscreen_unitst:is_instance(v.parent) then
   -- @unit    -- (v)unit -> z ; loo(k) -> enter ; (n)obles -> enter ; others
   u = v.parent.unit
  end
 elseif df.viewscreen_textviewerst:is_instance(v) then
  -- @textviewer    -- @unit -> enter (thoughts and preferences)
  if df.viewscreen_unitst:is_instance(v.parent) then
   -- @unit    -- @unit -> enter (thoughts and preferences)
   u = v.parent.unit
  elseif df.viewscreen_itemst:is_instance(v.parent) then
   tmp = v.parent.entry_ref[v.parent.cursor_pos]
   if df.general_ref_unit:is_instance(tmp) then -- general_ref_unit and derived ; general_ref_contains_unitst ; others?
    u = getUnit_byID(tmp.unit_id)
   end
  elseif df.viewscreen_dwarfmodest:is_instance(v.parent) then
   tmp = df.global.ui.main.mode
   if tmp == 24 then -- (v)iew units {g,i,p,w} -> z (thoughts and preferences)
    -- context: @dwarfmode/ViewUnits/...
    --if df.global.ui_selected_unit > -1 then -- -1 = 'no units nearby'
    u = df.global.world.units.active[df.global.ui_selected_unit]
   --end
   elseif tmp == 25 then -- loo(k) unit -> enter (thoughs and preferences)
    -- context: @dwarfmode/LookAround/Unit
    tmp = df.global.ui_look_list.items[df.global.ui_look_cursor]
    if tmp.type == 2 then -- 0:item 1:terrain >>2: unit<< 3:building 4:colony/vermin 7:spatter
     u = tmp.unit
    end
   end
  elseif df.viewscreen_unitlistst:is_instance(v.parent) then -- (u)nit list -> (v)iew unit (not citizen)
   -- context: @unitlist/Citizens ; @unitlist/Livestock ; @unitlist/Others ; @unitlist/Dead
   u = v.parent.units[v.parent.page][ v.parent.cursor_pos[v.parent.page] ]
  end
 end    -- switch viewscreen
 if not u and not silent then
  dfhack.printerr('No unit is selected in the UI or context not supported.')
 end
 return u
end -- getUnit_byVS()

--http://lua-users.org/wiki/StringRecipes ----------
function str2FirstUpper(str)
 return str:gsub("^%l", string.upper)
end

--------------------------------------------------
--http://lua-users.org/wiki/StringRecipes ----------
local function tchelper(first, rest)
 return first:upper()..rest:lower()
end

-- Add extra characters to the pattern if you need to. _ and ' are
--  found in the middle of identifiers and English words.
-- We must also put %w_' into [%w_'] to make it handle normal stuff
-- and extra stuff the same.
-- This also turns hex numbers into, eg. 0Xa7d4
function str2TitleCase(str)
 return str:gsub("(%a)([%w_']*)", tchelper)
end

--------------------------------------------------
--isBlank suggestion by http://stackoverflow.com/a/10330861
function isBlank(x)
    x = tostring(x) or ""
    -- returns (not not match_begin), _ = match_end => not not true , _ => true
    -- returns not not nil => false (no match)
    return not not x:find("^%s*$")
end

--http://lua-users.org/wiki/StringRecipes  (removed indents since I am not using them)
function wrap(str, limit)--, indent, indent1)
 --indent = indent or ""
 --indent1 = indent1 or indent
 local limit = limit or 72
 local here = 1 ---#indent1
 return str:gsub("(%s+)()(%S+)()",    --indent1..str:gsub(
  function(sp, st, word, fi)
  if fi-here > limit then
   here = st -- - #indent
   return "\n"..word --..indent..word
  end
 end)
end


--------------------------------------------------
---------------------- Time ----------------------
--------------------------------------------------
local TU_PER_DAY = 1200
--[[
if advmode then TU_PER_DAY = 86400 ? or only for cur_year_tick?
advmod_TU / 72 = ticks
--]]
local TU_PER_MONTH = TU_PER_DAY * 28
local TU_PER_YEAR = TU_PER_MONTH * 12

local MONTHS = {
 'Granite',
 'Slate',
 'Felsite',
 'Hematite',
 'Malachite',
 'Galena',
 'Limestone',
 'Sandstone',
 'Timber',
 'Moonstone',
 'Opal',
 'Obsidian',
}
Time = defclass(Time)
function Time:init(args)
 self.year = args.year or 0
 self.ticks = args.ticks or 0
end
function Time:getDays() -- >>float<< Days as age (including years)
 return self.year * 336 + (self.ticks / TU_PER_DAY)
end
function Time:getMonths() -- >>int<< Months as age (not including years)
 return math.floor (self.ticks / TU_PER_MONTH)
end
function Time:getMonthStr() -- Month as date
 return MONTHS[self:getMonths()+1] or 'error'
end
function Time:getDayStr() -- Day as date
 local d = math.floor ( (self.ticks % TU_PER_MONTH) / TU_PER_DAY ) + 1
 if d == 11 or d == 12 or d == 13 then
  d = tostring(d)..'th'
 elseif d % 10 == 1 then
  d = tostring(d)..'st'
 elseif d % 10 == 2 then
  d = tostring(d)..'nd'
 elseif d % 10 == 3 then
  d = tostring(d)..'rd'
 else
  d = tostring(d)..'th'
 end
 return d
end
--function Time:__add()
--end
function Time:__sub(other)
 if DEBUG then print(self.year,self.ticks) end
 if DEBUG then print(other.year,other.ticks) end
 if self.ticks < other.ticks then
  return Time{ year = (self.year - other.year - 1) , ticks = (TU_PER_YEAR + self.ticks - other.ticks) }
 else
  return Time{ year = (self.year - other.year) , ticks = (self.ticks - other.ticks) }
 end
end
--------------------------------------------------
--------------------------------------------------

--------------------------------------------------
-------------------- Identity --------------------
--------------------------------------------------
local SINGULAR = 0
local PLURAL = 1
--local POSSESSIVE = 2

local PRONOUNS = {
 [0]='She',
 [1]='He',
 [2]='It',
}
local BABY = 0
local CHILD = 1
local ADULT = 2

local TRAINING_LEVELS = {
 [0] = ' (Semi-Wild)',    -- Semi-wild
  ' (Trained)',            -- Trained
  ' (-Trained-)',            -- Well-trained
  ' (+Trained+)',            -- Skillfully trained
  ' (*Trained*)',            -- Expertly trained
  ' ('..string.char(240)..'Trained'..string.char(240)..')',    -- Exceptionally trained
  ' ('..string.char(15)..'Trained'..string.char(15)..')',        -- Masterully Trained
  ' (Tame)',                -- Domesticated
  '',                        -- undefined
  '',                        -- wild/untameable
}

local DEATH_TYPES = {
 [0] = ' died of old age',        -- OLD_AGE
  ' starved to death',            -- HUNGER
  ' died of dehydration',            -- THIRST
  ' was shot and killed',            -- SHOT
  ' bled to death',                -- BLEED
  ' drowned',                        -- DROWN
  ' suffocated',                    -- SUFFOCATE
  ' was struck down',                -- STRUCK_DOWN
  ' was scuttled',                -- SCUTTLE
  " didn't survive a collision",    -- COLLISION
  ' took a magma bath',            -- MAGMA
  ' took a magma shower',            -- MAGMA_MIST
  ' was incinerated by dragon fire',    -- DRAGONFIRE
  ' was killed by fire',            -- FIRE
  ' experienced death by SCALD',    -- SCALD
  ' was crushed by cavein',        -- CAVEIN
  ' was smashed by a drawbridge',    -- DRAWBRIDGE
  ' was killed by falling rocks',    -- FALLING_ROCKS
  ' experienced death by CHASM',    -- CHASM
  ' experienced death by CAGE',    -- CAGE
  ' was murdered',                -- MURDER
  ' was killed by a trap',    -- TRAP
  ' vanished',                    -- VANISH
  ' experienced death by QUIT',    -- QUIT
  ' experienced death by ABANDON',    -- ABANDON
  ' suffered heat stroke',        -- HEAT
  ' died of hypothermia',            -- COLD
  ' experienced death by SPIKE',    -- SPIKE
  ' experienced death by ENCASE_LAVA',    -- ENCASE_LAVA
  ' experienced death by ENCASE_MAGMA',    -- ENCASE_MAGMA
  ' was preserved in ice',        -- ENCASE_ICE
  ' became headless',                -- BEHEAD
  ' was crucified',                -- CRUCIFY
  ' experienced death by BURY_ALIVE',    -- BURY_ALIVE
  ' experienced death by DROWN_ALT',    -- DROWN_ALT
  ' experienced death by BURN_ALIVE',    -- BURN_ALIVE
  ' experienced death by FEED_TO_BEASTS',    -- FEED_TO_BEASTS
  ' experienced death by HACK_TO_PIECES',    -- HACK_TO_PIECES
  ' choked on air',                -- LEAVE_OUT_IN_AIR
  ' experienced death by BOIL',    -- BOIL
  ' melted',                        -- MELT
  ' experienced death by CONDENSE',    -- CONDENSE
  ' experienced death by SOLIDIFY',    -- SOLIDIFY
  ' succumbed to infection',        -- INFECTION
  "'s ghost was put to rest with a memorial",    -- MEMORIALIZE
  ' scared to death',                -- SCARE
  ' experienced death by DARKNESS',    -- DARKNESS
  ' experienced death by COLLAPSE',    -- COLLAPSE
  ' was drained of blood',            -- DRAIN_BLOOD
  ' was slaughtered',                -- SLAUGHTER
  ' became roadkill',                -- VEHICLE
  ' killed by a falling object',    -- FALLING_OBJECT
}

--GHOST_TYPES[unit.relations.ghost_info.type].."  This spirit has not been properly memorialized or buried."
local GHOST_TYPES = {
 [0]="A murderous ghost.",
  "A sadistic ghost.",
  "A secretive ghost.",
  "An energetic poltergeist.",
  "An angry ghost.",
  "A violent ghost.",
  "A moaning spirit returned from the dead.  It will generally trouble one unfortunate at a time.",
  "A howling spirit.  The ceaseless noise is making sleep difficult.",
  "A troublesome poltergeist.",
  "A restless haunt, generally troubling past acquaintances and relatives.",
  "A forlorn haunt, seeking out known locations or drifting around the place of death.",
}


Identity = defclass(Identity)
function Identity:init(args)
 local u = args.unit
 self.ident = dfhack.units.getIdentity(u)

 self.unit = u
 self.name = dfhack.TranslateName( dfhack.units.getVisibleName(u) )
 self.name_en = dfhack.TranslateName( dfhack.units.getVisibleName(u) , true)
 self.raw_prof = dfhack.units.getProfessionName(u)
 self.pronoun = PRONOUNS[u.sex] or 'It'

 if self.ident then
  self.birth_date = Time{year = self.ident.birth_year, ticks = self.ident.birth_second}
  self.race_id = self.ident.race
  self.caste_id = self.ident.caste
  if self.ident.histfig_id > -1 then
   self.hf_id = self.ident.histfig_id
  end
 else
  self.birth_date = Time{year = self.unit.relations.birth_year, ticks = self.unit.relations.birth_time}
  self.race_id = u.race
  self.caste_id = u.caste
  if u.hist_figure_id > -1 then
   self.hf_id = u.hist_figure_id
  end
 end
 self.race = df.global.world.raws.creatures.all[self.race_id]
 self.caste = self.race.caste[self.caste_id]

 self.isCivCitizen = (df.global.ui.civ_id == u.civ_id)
 self.isStray = u.flags1.tame --self.isCivCitizen and not u.flags1.merchant
 self.cur_date = Time{year = df.global.cur_year, ticks = df.global.cur_year_tick}


 ------------ death ------------
 self.dead = u.flags1.dead
 self.ghostly = u.flags3.ghostly
 self.undead = u.enemy.undead

 if self.dead and self.hf_id then -- dead-dead not undead-dead
  local events = df.global.world.history.events2
  local e
  for idx = #events - 1,0,-1 do
   e = events[idx]
   if df.history_event_hist_figure_diedst:is_instance(e) and e.victim_hf == self.hf_id then
    self.death_event = e
    break
   end
  end
 end
  if u.counters.death_id > -1 then -- if undead/ghostly dead or dead-dead
   self.incident = df.global.world.incidents.all[u.counters.death_id]
   if not self.incident.flags.discovered then
    self.missing = true
   end
  end
  -- slaughtered?
  if self.death_event then
   self.death_date = Time{year = self.death_event.year, ticks = self.death_event.seconds}
  elseif self.incident then
   self.death_date = Time{year = self.incident.event_year, ticks = self.incident.event_time}
  end
  -- age now or age death?
  if self.dead and self.death_date then -- if cursed with no age? -- if hacked a ressurection, such that they aren't dead anymore, don't use the death date
   self.age_time = self.death_date - self.birth_date
  else
   self.age_time = self.cur_date - self.birth_date
  end
 if DEBUG then print( self.age_time.year,self.age_time.ticks,self.age_time:getMonths() ) end
 ---------- ---------- ----------


    ---------- caste_name ----------
 self.caste_name = {}
 if isBlank(self.caste.caste_name[SINGULAR]) then
  self.caste_name[SINGULAR] = self.race.name[SINGULAR]
 else
  self.caste_name[SINGULAR] = self.caste.caste_name[SINGULAR]
 end
 if isBlank(self.caste.caste_name[PLURAL]) then
  self.caste_name[PLURAL] = self.race.name[PLURAL]
 else
  self.caste_name[PLURAL] = self.caste.caste_name[PLURAL]
 end
 ---------- ---------- ----------

 --------- growth_status ---------
 -- 'baby_age' is the age the baby becomes a child
 -- 'child_age' is the age the child becomes an adult
 if self.age_time.year >= self.caste.misc.child_age then -- has child come of age becoming adult?
  self.growth_status = ADULT
 elseif self.age_time.year >= self.caste.misc.baby_age then -- has baby come of age becoming child?
  self.growth_status = CHILD
 else
  self.growth_status = BABY
 end
 ---------- ---------- ----------

 -------- aged_caste_name --------
 local caste_name, race_name
 if self.growth_status == ADULT then
  caste_name = self.caste.caste_name[SINGULAR]
  race_name = self.race.name[SINGULAR]
 elseif self.growth_status == CHILD then
  caste_name = self.caste.child_name[SINGULAR]
  race_name = self.race.general_child_name[SINGULAR]
 else --if self.growth_status == BABY then
  caste_name = self.caste.baby_name[SINGULAR]
  race_name = self.race.general_baby_name[SINGULAR]
 end
 self.aged_caste_name = {}
 if isBlank(caste_name[SINGULAR]) then
  self.aged_caste_name[SINGULAR] = race_name[SINGULAR]
 else
  self.aged_caste_name[SINGULAR] = caste_name[SINGULAR]
 end
 if isBlank(caste_name[PLURAL]) then
  self.aged_caste_name[PLURAL] = race_name[PLURAL]
 else
  self.aged_caste_name[PLURAL] = caste_name[PLURAL]
 end
 ---------- ---------- ----------

 ----- Profession adjustment -----
 local prof = self.raw_prof
 if self.undead then
  prof = str2TitleCase( self.caste_name[SINGULAR] )
  if isBlank(u.enemy.undead.anon_7) then
   prof = prof..' Corpse'
  else
   prof = u.enemy.undead.anon_7 -- a reanimated body part will use this string instead
  end
 end
 --[[
 if self.ghostly then
  prof = 'Ghostly '..prof
 end
 --]]
 if u.curse.name_visible and not isBlank(u.curse.name) then
  prof = prof..' '..u.curse.name
 end
 if isBlank(self.name) then
  if self.isStray then
   prof = 'Stray '..prof --..TRAINING_LEVELS[u.training_level]
  end
 end
 self.prof = prof
 ---------- ---------- ----------
end
--------------------------------------------------
--------------------------------------------------
--[[
 prof_id ?
 group_id ?
 fort_race_id
 fort_civ_id
 --fort_group_id?
--]]


UnitInfoViewer = defclass(UnitInfoViewer, gui.FramedScreen)
UnitInfoViewer.focus_path = 'unitinfoviewer' -- -> dfhack/lua/unitinfoviewer
UnitInfoViewer.ATTRS={
 frame_style = gui.GREY_LINE_FRAME,
 frame_inset = 2, -- used by init
 frame_outset = 1,--3, -- new, used by init; 0 = full screen, suggest 0, 1, or 3 or maybe 5
 --frame_title , -- not used
 --frame_width,frame_height calculated by frame inset and outset in init
}
function UnitInfoViewer:init(args) -- requires args.unit
 --if DEBUG then print('-----') end
 local x,y = dfhack.screen.getWindowSize()
 -- what if inset or outset are defined as {l,r,t,b}?
 x = x - 2*(self.frame_inset + 1 + self.frame_outset) -- 1=frame border thickness
 y = y - 2*(self.frame_inset + 1 + self.frame_outset) -- 1=frame border thickness
 self.frame_width = args.frame_width or x
 self.frame_height = args.frame_height or y
 self.text = {}
 if df.unit:is_instance(args.unit) then
  self.ident = Identity{ unit = args.unit }
  if not isBlank(self.ident.name_en) then
   self.frame_title = 'Unit: '..self.ident.name_en
  elseif not isBlank(self.ident.prof) then
   self.frame_title = 'Unit: '..self.ident.prof
   if self.ident.isStray then
    self.frame_title = self.frame_title..TRAINING_LEVELS[self.ident.unit.training_level]
   end
  end
  self:chunk_Name()
  self:chunk_Description()
  if not (self.ident.dead or self.ident.undead or self.ident.ghostly) then --not self.dead
   if self.ident.isCivCitizen then
    self:chunk_Age()
    self:chunk_MaxAge()
   end
   if self.ident.isStray then
    if self.ident.growth_status == ADULT then
     self:chunk_Milkable()
    end
    self:chunk_Grazer()
    if self.ident.growth_status == ADULT then
     self:chunk_Shearable()
    end
    if self.ident.growth_status == ADULT then
     self:chunk_EggLayer()
    end
   end
   self:chunk_BodySize()
  elseif self.ident.ghostly then
   self:chunk_Dead()
   self:chunk_Ghostly()
  elseif self.ident.undead then
   self:chunk_BodySize()
   self:chunk_Dead()
  else
   self:chunk_Dead()
  end
 else
  self:insert_chunk("No unit is selected in the UI or context not supported.",pens.LIGHTRED)
 end
 self:addviews{    widgets.Label{    frame={yalign=0}, text=self.text    }    }
end
function UnitInfoViewer:onInput(keys)
 if keys.LEAVESCREEN or keys.SELECT then
  self:dismiss()
 end
end
function UnitInfoViewer:onGetSelectedUnit()
 return self.ident.unit
end
function UnitInfoViewer:insert_chunk(str,pen)
 local lines = utils.split_string( wrap(str,self.frame_width) , NEWLINE )
 for i = 1,#lines do
  table.insert(self.text,{text=lines[i],pen=pen})
  table.insert(self.text,NEWLINE)
 end
 table.insert(self.text,NEWLINE)
end
function UnitInfoViewer:chunk_Name()
 local i = self.ident
 local u = i.unit
 local prof = i.prof
 local color = dfhack.units.getProfessionColor(u)
 local blurb
 if i.ghostly then
  prof = 'Ghostly '..prof
 end
 if i.isStray then
  prof = prof..TRAINING_LEVELS[u.training_level]
 end
 if isBlank(i.name) then
  if isBlank(prof) then
   blurb = 'I am a mystery'
  else
   blurb = prof
  end
 else
  if isBlank(prof) then
   blurb=i.name
  else
   blurb=i.name..', '..prof
  end
 end
 self:insert_chunk(blurb,dfhack.pen.parse{fg=color,bg=0})
end
function UnitInfoViewer:chunk_Description()
 local dsc = self.ident.caste.description
 if not isBlank(dsc) then
  self:insert_chunk(dsc,pens.WHITE)
 end
end

function UnitInfoViewer:chunk_Age()
 local i = self.ident
 local age_str -- = ''
 if i.age_time.year > 1 then
  age_str = tostring(i.age_time.year)..' years old'
 elseif i.age_time.year > 0 then -- == 1
  age_str = '1 year old'
 else --if age_time.year == 0 then
  local age_m = i.age_time:getMonths() -- math.floor
  if age_m > 1 then
   age_str = tostring(age_m)..' months old'
  elseif age_m > 0 then -- age_m == 1
   age_str = '1 month old'
  else -- if age_m == 0 then -- and age_m < 0 which would be an error
   age_str = 'a newborn'
  end
 end
 local blurb = i.pronoun..' is '..age_str
 if i.race_id == df.global.ui.race_id then
  blurb = blurb..', born on the '..i.birth_date:getDayStr()..' of '..i.birth_date:getMonthStr()..' in the year '..tostring(i.birth_date.year)..PERIOD
 else
  blurb = blurb..PERIOD
 end
 self:insert_chunk(blurb,pens.YELLOW)
end

function UnitInfoViewer:chunk_MaxAge()
 local i = self.ident
 local maxage = math.floor( (i.caste.misc.maxage_max + i.caste.misc.maxage_min)/2 )
 --or i.unit.curse.add_tags1.NO_AGING   hidden ident?
 if i.caste.misc.maxage_min == -1 then
  maxage = ' die of unnatural causes.'
 elseif maxage == 0 then
  maxage = ' die at a very young age.'
 elseif maxage == 1 then
  maxage = ' live about '..tostring(maxage)..' year.'
 else
  maxage = ' live about '..tostring(maxage)..' years.'
 end
 --' is expected to '..
 local blurb = str2FirstUpper(i.caste_name[PLURAL])..maxage
 self:insert_chunk(blurb,pens.DARKGREY)
end
function UnitInfoViewer:chunk_Grazer()
 if self.ident.caste.flags.GRAZER then
  local blurb = 'Grazing satisfies '..tostring(self.ident.caste.misc.grazer)..' units of hunger.'
  self:insert_chunk(blurb,pens.LIGHTGREEN)
 end
end
function UnitInfoViewer:chunk_EggLayer()
 local caste = self.ident.caste
 if caste.flags.LAYS_EGGS then
  local clutch = math.floor( (caste.misc.clutch_size_max + caste.misc.clutch_size_min)/2 )
  local blurb = 'Lays clutches of about '..tostring(clutch)
  if clutch > 1 then
   blurb = blurb..' eggs.'
  else
   blurb = blurb..' egg.'
  end
  self:insert_chunk(blurb,pens.GREEN)
 end
end
function UnitInfoViewer:chunk_Milkable()
 local i = self.ident
 if i.caste.flags.MILKABLE then
  local milk = dfhack.matinfo.decode( i.caste.extracts.milkable_mat , i.caste.extracts.milkable_matidx )
  if milk then
   local days,seconds = math.modf ( i.caste.misc.milkable / TU_PER_DAY )
   days = (seconds > 0) and (tostring(days)..' to '..tostring(days + 1)) or tostring(days)
   --local blurb = pronoun..' produces '..milk:toString()..' every '..days..' days.'
   local blurb = (i.growth_status == ADULT) and (i.pronoun..' secretes ') or str2FirstUpper(i.caste_name[PLURAL])..' secrete '
   blurb = blurb..milk:toString()..' every '..days..' days.'
   self:insert_chunk(blurb,pens.LIGHTCYAN)
  end
 end
end
function UnitInfoViewer:chunk_Shearable()
 local i = self.ident
 local mat_types = i.caste.body_info.materials.mat_type
 local mat_idxs = i.caste.body_info.materials.mat_index
 local mat_info, blurb
 for idx,mat_type in ipairs(mat_types) do
  mat_info = dfhack.matinfo.decode(mat_type,mat_idxs[idx])
  if mat_info and mat_info.material.flags.YARN then
   blurb = (i.growth_status == ADULT) and (i.pronoun..' produces ') or str2FirstUpper(i.caste_name[PLURAL])..' produce '
   blurb = blurb..mat_info:toString()..PERIOD
   self:insert_chunk(blurb,pens.BROWN)
  end
 end
end
function UnitInfoViewer:chunk_BodySize()
 local i = self.ident
 local pat = i.unit.body.physical_attrs
 local blurb = i.pronoun..' appears to be about '..pat.STRENGTH.value..':'..pat.AGILITY.value..' cubic decimeters in size.'
 self:insert_chunk(blurb,pens.LIGHTBLUE)
end
function UnitInfoViewer:chunk_Ghostly()
 local blurb = GHOST_TYPES[self.ident.unit.relations.ghost_info.type].."  This spirit has not been properly memorialized or buried."
 self:insert_chunk(blurb,pens.LIGHTMAGENTA)
 -- Arose in relations.curse_year curse_time
end
function UnitInfoViewer:chunk_Dead()
 local i = self.ident
 local blurb, str, pen
 if i.missing then --dfhack.units.isDead(unit)
  str = ' is missing.'
  pen = pens.WHITE
 elseif i.death_event then
  --str = "The Caste_name Unit_Name died in year #{e.year}"
  --str << " (cause: #{e.death_cause.to_s.downcase}),"
  --str << " killed by the #{e.slayer_race_tg.name[0]} #{e.slayer_hf_tg.name}" if e.slayer_hf != -1
  --str << " using a #{df.world.raws.itemdefs.weapons[e.weapon.item_subtype].name}" if e.weapon.item_type == :WEAPON
  --str << ", shot by a #{df.world.raws.itemdefs.weapons[e.weapon.bow_item_subtype].name}" if e.weapon.bow_item_type == :WEAPON
  str = DEATH_TYPES[i.death_event.death_cause]..PERIOD
  pen = pens.MAGENTA
 elseif i.incident then
  --str = "The #{u.race_tg.name[0]}"
  --str << " #{u.name}" if u.name.has_name
  --str << " died"
  --str << " in year #{incident.event_year}" if incident
  --str << " (cause: #{u.counters.death_cause.to_s.downcase})," if u.counters.death_cause != -1
  --str << " killed by the #{killer.race_tg.name[0]} #{killer.name}" if killer
  str = DEATH_TYPES[i.incident.death_cause]..PERIOD
  pen = pens.MAGENTA
 elseif i.unit.flags2.slaughter and i.unit.flags2.killed then
  str = ' was slaughtered.'
  pen = pens.MAGENTA
 else
  str = ' is dead.'
  pen = pens.MAGENTA
 end
 if i.undead or i.ghostly then
  str = ' is undead.'
  pen = pens.GREY
 end
 blurb = 'The '..i.prof -- assume prof is not blank
 if not isBlank(i.name) then
  blurb = blurb..', '..i.name
 end
 blurb = blurb..str
 self:insert_chunk(blurb,pen)
end

-- only show if UnitInfoViewer isn't the current focus
if dfhack.gui.getCurFocus() ~= 'dfhack/lua/'..UnitInfoViewer.focus_path then
 local gui_no_unit = false -- show if not found?
 local unit = getUnit_byVS(gui_no_unit) -- silent? or let the gui display
 if unit or gui_no_unit then
  local kan_viewscreen = UnitInfoViewer{unit = unit}
  kan_viewscreen:show()
 end
end

