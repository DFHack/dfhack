function err(msg) --make local maybe...
	print(msg)
	print(debug.traceback())
end
function dofile(filename) --safer dofile, with traceback (very usefull)
	f,perr=loadfile(filename)
	if f~=nil then
		return xpcall(f,err)
	else
		print(perr)
	end
end
dofile("dfusion/common.lua")

print("Unlocking Df .text section...")
unlockDF()
print("Done unlock")
lockDF()
dofile("dfusion/simple_embark/plugin.lua")
print("hello world")
Console.print("Hello world in console!\n")
--name=Console.lineedit("Enter name:")
--Console.print("Your name is:"..name)

function OnTick() -- floods the console
	r=Console.get_rows()
	c=Console.get_columns()
	Console.clear()
	Console.gotoxy(math.random(1,r),math.random(1,2))
	Console.color(math.random(0,15))
	Console.print("*")
end
OnTick=nil