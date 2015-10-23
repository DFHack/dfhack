--makes it so that civs won't come with barrels full of blood, ichor, or goo
--author Urist Da Vinci
--edited by expwnent, scamtank
--[[=begin

fix/blood-del
=============
Makes it so that future caravans won't bring barrels full of blood, ichor, or goo.

=end]]
local my_entity=df.historical_entity.find(df.global.ui.civ_id)
local sText=" "
local k=0
local v=1

for x,y in pairs(df.global.world.entities.all) do
 my_entity=y
 k=0
 while k < #my_entity.resources.misc_mat.extracts.mat_index do
  v=my_entity.resources.misc_mat.extracts.mat_type[k]
  sText=dfhack.matinfo.decode(v,my_entity.resources.misc_mat.extracts.mat_index[k])
  if (sText==nil) then
   --LIQUID barrels
   my_entity.resources.misc_mat.extracts.mat_type:erase(k)
   my_entity.resources.misc_mat.extracts.mat_index:erase(k)
   k=k-1
  else
   if(sText.material.id=="BLOOD") then
    my_entity.resources.misc_mat.extracts.mat_type:erase(k)
    my_entity.resources.misc_mat.extracts.mat_index:erase(k)
    k=k-1
   end
   if(sText.material.id=="ICHOR") then
    my_entity.resources.misc_mat.extracts.mat_type:erase(k)
    my_entity.resources.misc_mat.extracts.mat_index:erase(k)
    k=k-1
   end
   if(sText.material.id=="GOO") then
    my_entity.resources.misc_mat.extracts.mat_type:erase(k)
    my_entity.resources.misc_mat.extracts.mat_index:erase(k)
    k=k-1
   end
   if(sText.material.id=="SWEAT") then
    my_entity.resources.misc_mat.extracts.mat_type:erase(k)
    my_entity.resources.misc_mat.extracts.mat_index:erase(k)
    k=k-1
   end
   if(sText.material.id=="TEARS") then
    my_entity.resources.misc_mat.extracts.mat_type:erase(k)
    my_entity.resources.misc_mat.extracts.mat_index:erase(k)
    k=k-1
   end
   --VENOM
   --POISON
   --FLUID
   --MILK
   --EXTRACT

  end
  k=k+1
 end
end

