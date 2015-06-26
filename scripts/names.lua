local utils = require 'utils'

validArgs = validArgs or utils.invert({
 'help',
 'item',
 'unit',
 'first'
})

local args = utils.processArgs({...}, validArgs)

if args.help then
 print(
[[names.lua
arguments:
    -help				
        print this help message
    -item 					
	if viewing an item		
    -unit 					
	if viewing a unit 			
    -first Somename or "Some Names like This"
	if a first name is desired, leave blank to clear current first name
]])
 return
end

if not df.viewscreen_layer_choose_language_namest:is_instance(dfhack.gui.getCurViewscreen()) then
	choices = df.new(df.viewscreen_setupadventurest) ; choices.subscreen = 3 ; gui = require 'gui' ; gui.simulateInput(choices, 'A_RANDOM_NAME') ; gui.simulateInput(choices, 'A_CUST_NAME')
end

if args.item then
	fact = dfhack.gui.getCurViewscreen().parent.item.general_refs[0].artifact_id
	trg = df.artifact_record.find(fact)
end
 
if args.unit then
	trg = dfhack.gui.getCurViewscreen().parent.unit
end

if args.first then
	trg.name.first_name = args.first
end

function newName()
 	newn = dfhack.gui.getCurViewscreen().name
	oldn = trg.name
	for k = 0,6 do
	 	oldn.words[k] = newn.words[k]
 		oldn.parts_of_speech[k] = newn.parts_of_speech[k]
	end
 	oldn.language = newn.language
 	oldn.has_name = newn.has_name
end
newName()
