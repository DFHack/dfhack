func={}
dofile("dfusion/triggers/functions.lua")
func.menu=MakeMenu()
function func.PrintMessage_()
	print("Type a message:")
	msg=io.stdin:read()
	func.PrintMessage(msg,6,1)
end
if not(FILE) then -- if not in script mode
	func.menu:add("Print message",func.PrintMessage_)
	func.menu:display()
end