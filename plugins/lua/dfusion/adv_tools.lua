local _ENV = mkmodule('plugins.dfusion.adv_tools')
local dfu=require("plugins.dfusion")
menu=dfu.SimpleMenu()
function Reincarnate(trg_unit,swap_soul) --only for adventurer i guess
	if swap_soul==nil then
		swap_soul=true
	end
	local adv=trg_unit or df.global.world.units.active[0]
	if adv.flags1.dead==false then
		qerror("You are not dead (yet)!")
	end
	local hist_fig=getNemesis(adv).figure
	if hist_fig==nil then
		qerror("No historical figure for adventurer...")
	end
	local events=df.global.world.history.events
	local trg_hist_fig
	for i=#events-1,0,-1 do -- reverse search because almost always it will be last entry
		if df.history_event_hist_figure_diedst:is_instance(events[i]) then
			--print("is instance:"..i)
			if events[i].victim==hist_fig.id then
				--print("Is same id:"..i)
				trg_hist_fig=events[i].slayer
				if trg_hist_fig then
					trg_hist_fig=df.historical_figure.find(trg_hist_fig)
				end
				break
			end
		end
	end
	if trg_hist_fig ==nil then
		qerror("Slayer not found")
	end
	
	local trg_unit=trg_hist_fig.unit_id
	if trg_unit==nil then
		qerror("Unit id not found!")
	end
	local trg_unit_final=df.unit.find(trg_unit)
	
	tools.change_adv(trg_unit_final)
	if swap_soul then --actually add a soul...
		t_soul=adv.status.current_soul
		adv.status.current_soul=df.NULL
		adv.status.souls:resize(0)
		trg_unit_final.status.current_soul=t_soul
		trg_unit_final.status.souls:insert(#trg_unit_final.status.souls,t_soul)
	end
end
menu:add("Reincarnate",Reincarnate,{{df.unit,"optional"}})-- bool, optional

return _ENV