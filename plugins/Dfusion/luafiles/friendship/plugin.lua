if not(FILE) then
	--sanity test
	--print("race num:"..engine.peekw(offsets.getEx("CurrentRace")))
	--print(string.format("%x vs %x",offsets.getEx("CurrentRace"),VersionInfo.getGroup("Creatures"):getAddress("current_race")))
	print("Race num:"..df.global.ui.race_id)
	print("Your current race is:"..GetRaceToken(df.global.ui.race_id))
	print("If this is wrong please type 'q'")
	if(getline()=='q') then
		return
	end

end

if not(FILE) then
	names=ParseNames("dfusion/friendship/races.txt")--io.open("plugins/friendship/races.txt"):lines()
	friendship_in.install(names)
	friendship_in.patch()
end