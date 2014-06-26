-- skillChange.lua
-- Allows skills to be changed with DFHack.
-- author Putnam
-- edited by expwnent

--[[
Args are arranged like this:
unitID SKILL level ADD/SUBTRACT/SET

SKILL being anything such as DETAILSTONE, level being a number that it will be set to (15 is legendary, 0 is dabbling).

Add will add skill levels, subtract will subtract, set will place it at exactly the level you put in.
]]

local utils=require('utils')
local args={...}
local skills=df.unit.find(args[1]).status.current_soul.skills
local skill=df.job_skill[tostring(args[2]):upper()]
local level=tonumber(args[3]) or 0

local argfunctions={
 __index=function(t,k)
  qerror(k..' is not a valid setting! Valid settings are SET, ADD and SUBTRACT.')
 end,
 set=function(skills,skill,level)
  utils.insert_or_update(skills, {new=true, id=skill, rating=level}, 'id')
 end,
 add=function(skills,skill,level)
  local skillExists,oldSkill=utils.linear_index(skills,skill,'id')
  if skillExists then 
   oldSkill.rating=level+oldSkill.rating
  else
   argfunctions.set(skills,skill,level)
  end
 end,
 subtract=function(skills,skill,level)
  local skillExists,oldSkill=utils.linear_index(skills,skill,'id')
  if skillExists then 
   local newRating=oldSkill['rating'] or 0
   oldSkill.rating=oldSkill.rating-level
   if oldSkill.rating<0 or (oldSkill.rating==0 and oldSkill.experience<=0) then skills:erase(skillExists) end
  end
 end
}

setmetatable(argfunctions,argfunctions)
argfunctions[args[4]:lower()](skills,skill,level)
