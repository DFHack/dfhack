Dfusion plugin offers four DFhack commands: 'dfusion', 'dfuse' and 'lua', 'runlua'.
lua:
Runs an interactive lua console. For more on lua commands see [http://www.lua.org/manual/5.1/manual.html Lua reference manual] or google "lua". Also this command could be ran with filepath as an argument. Then it runs that file as a lua script file. E.g. ''lua dfusion/temp.lua'' runs a file  <your df path>/dfusion/temp.lua.
runlua:
Similar to ''lua <filename>'' but not interactive, to be used with hotkeys
dfusion:
First this command runs all plugins' init.lua part then show a menu. Type number to run specified plugin.
dfuse:
Similar to dfusion but not interactive. To be used with hotkeys (later will have command support).

Also dfuse/dfusion runs an init script located at 'save directory/dfusion/init.lua'. And 'initcustom.lua' if it exists
More info http://dwarffortresswiki.org/index.php/Utility:DFusion
