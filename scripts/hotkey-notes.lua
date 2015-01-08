-- prints info on assigned hotkeys to the console

local h_list = {'F1', 'F2', 'F3', 'F4', 'F5', 'F6', 'F7', 'F8',
                'Shift+F1', 'Shift+F2', 'Shift+F3', 'Shift+F4',
				'Shift+F5', 'Shift+F6', 'Shift+F7', 'Shift+F8'}

for i=1, #df.global.ui.main.hotkeys do
    local hk = df.global.ui.main.hotkeys[i-1]
	if hk.cmd ~= -1 then
		print(h_list[i]..': '..hk.name..':  x='..hk.x..'  y='..hk.y..'  z='..hk.z)
	end
end
