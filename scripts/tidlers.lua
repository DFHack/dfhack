--Toggle idlers count
--[[=begin

tidlers
=======
Toggle between all possible positions where the idlers count can be placed.

=end]]
-- see also the enum: df.d_init_idlers
if df.global.d_init.idlers == 0 then
    df.global.d_init.idlers = 1
elseif df.global.d_init.idlers == 1 then
    df.global.d_init.idlers = -1
else
    df.global.d_init.idlers = 0
end
print('Toggled the display of idlers.')
