-- prints info on assigned hotkeys to the console

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

--[[ 
# the (very) old Python version...
from context import Context, ContextManager

cm = ContextManager("Memory.xml")
df = cm.get_single_context()

df.attach()

gui = df.gui

print "Hotkeys"

hotkeys = gui.read_hotkeys()

for key in hotkeys:
    print "x: %d\ny: %d\tz: %d\ttext: %s" % (key.x, key.y, key.z, key.name)

df.detach()

print "Done.  Press any key to continue"
raw_input()
]]--
