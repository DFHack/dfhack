-- Toggle display of water depth
--[[=begin

twaterlvl
=========
Toggle between displaying/not displaying liquid depth as numbers.

=end]]

df.global.d_init.flags1.SHOW_FLOW_AMOUNTS = not df.global.d_init.flags1.SHOW_FLOW_AMOUNTS
print('Water level display toggled.')
