-- prints info on assigned hotkeys to the console
--[[=begin

hotkey-notes
============
Lists the key, name, and jump position of your hotkeys in the DFHack console.

=end]]

for i=1, #df.global.ui.main.hotkeys do
    local hk = df.global.ui.main.hotkeys[i-1]
    local key = dfhack.screen.getKeyDisplay(df.interface_key.D_HOTKEY1 + i - 1)
    if hk.cmd ~= -1 then
        print(key..': '..hk.name..':  x='..hk.x..'  y='..hk.y..'  z='..hk.z)
    end
end
