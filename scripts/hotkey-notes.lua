-- prints info on assigned hotkeys to the console

-- A work in progress, hoping for some help!
-- A different approach might be needed depending on internal structures, 
--    but this gets the idea across.

local hotkeys = {'F1 ', 'F2 ', 'F3 ', 'F4 ', 'F5 ', 'F6 ',
                 'F7 ', 'F8 ', 'F9 ', 'F10', 'F11', 'F12'}

for i=1, #hotkeys do
    local hk = hotkeys[i]
    hk = {id=hk}
    -- PLACEHOLDER PROPERTIES ONLY!
    hk.name = '_name'
    hk.x = df.global.window_x
    hk.y = df.global.window_y
    hk.z = df.global.window_z

    print(hk.id..'  '..hk.name..'    X= '..hk.x..',  Y= '..hk.y..',  Z= '..hk.z)
end