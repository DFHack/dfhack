-- gui/family-affairs
-- derived from v1.2 @ http://www.bay12forums.com/smf/index.php?topic=147779
local helpstr = [====[

gui/family-affairs
==================
A user-friendly interface to view romantic relationships,
with the ability to add, remove, or otherwise change them at
your whim - fantastic for depressed dwarves with a dead spouse
(or matchmaking players...).

The target/s must be alive, sane, and in fortress mode.

.. image:: /docs/images/family-affairs.png
   :align: center

``gui/family-affairs [unitID]``
        shows GUI for the selected unit, or the specified unit ID

``gui/family-affairs divorce [unitID]``
        removes all spouse and lover information from the unit
        and it's partner, bypassing almost all checks.

``gui/family-affairs [unitID] [unitID]``
        divorces the two specified units and their partners,
        then arranges for the two units to marry, bypassing
        almost all checks.  Use with caution.

]====]

local dlg = require ('gui.dialogs')

function ErrorPopup (msg,color)
    if not tostring(msg) then msg = "Error" end
    if not color then color = COLOR_LIGHTRED end
    dlg.showMessage("Dwarven Family Affairs", msg, color, nil)
end

function AnnounceAndGamelog(text)
    dfhack.gui.showAnnouncement(text, COLOR_LIGHTMAGENTA)
end

function ListPrompt (msg, choicelist, bool, yes_func)
dlg.showListPrompt(
    "Dwarven Family Affairs",
    msg,
    COLOR_WHITE,
    choicelist,
    --called if choice is yes
    yes_func,
    --called on cancel
    function() end,
    15,
    bool
    )
end

function GetMarriageSummary (source)
    local familystate = ""
    local source_name = dfhack.TranslateName(source.name)

    local spouse = df.unit.find(source.relationship_ids.Spouse)
    local lover = df.unit.find(source.relationship_ids.Lover)

    if spouse then
        if dfhack.units.isSane(spouse) then
            familystate = source_name.." has a spouse ("..dfhack.TranslateName(spouse.name)..")"
        end
        if dfhack.units.isSane(spouse) == false then
            familystate = source_name.."'s spouse is dead or not sane, would you like to choose a new one?"
        end
    elseif lover then
        if dfhack.units.isSane(df.unit.find(source.relationship_ids.Lover)) then
            familystate = source_name.." already has a lover ("..dfhack.TranslateName(lover.name)..")"
        end
        if dfhack.units.isSane(df.unit.find(source.relationship_ids.Lover)) == false then
            familystate = source_name.."'s lover is dead or not sane, would you like that love forgotten?"
        end
    else
        familystate = source_name.." is not involved in romantic relationships with anyone"
    end

    if source.pregnancy_timer > 0 then
        familystate = familystate.."\nShe is pregnant."
        local father = df.historical_figure.find(source.pregnancy_spouse)
        if father then
            familystate = familystate.." The father is "..dfhack.TranslateName(father.name).."."
        end
    end

    return familystate
end

function GetSpouseData (source)
    local spouse = df.unit.find(source.relationship_ids.Spouse)
    local spouse_hf
    if spouse then
        spouse_hf = df.historical_figure.find(spouse.hist_figure_id)
    end
    return spouse,spouse_hf
end

function GetLoverData (source)
    local lover = df.unit.find(source.relationship_ids.Spouse)
    local lover_hf
    if lover then
        lover_hf = df.historical_figure.find (lover.hist_figure_id)
    end
    return lover,lover_hf
end

function EraseHFLinksLoverSpouse (hf)
    for i = #hf.histfig_links-1,0,-1 do
        if hf.histfig_links[i]._type == df.histfig_hf_link_spousest or hf.histfig_links[i]._type == df.histfig_hf_link_loverst then
            local todelete = hf.histfig_links[i]
            hf.histfig_links:erase(i)
            todelete:delete()
        end
    end
end

function Divorce (source)
    local source_hf = df.historical_figure.find(source.hist_figure_id)
    local spouse,spouse_hf = GetSpouseData (source)
    local lover,lover_hf = GetLoverData (source)

    source.relationship_ids.Spouse = -1
    source.relationship_ids.Lover = -1

    if source_hf then
        EraseHFLinksLoverSpouse (source_hf)
    end
    if spouse then
        spouse.relationship_ids.Spouse = -1
        spouse.relationship_ids.Lover = -1
    end
    if lover then
        spouse.relationship_ids.Spouse = -1
        spouse.relationship_ids.Lover = -1
    end
    if spouse_hf then
        EraseHFLinksLoverSpouse (spouse_hf)
    end
    if lover_hf then
        EraseHFLinksLoverSpouse (lover_hf)
    end

    local partner = spouse or lover
    if not partner then
        AnnounceAndGamelog(dfhack.TranslateName(source.name).." is now single")
    else
        AnnounceAndGamelog(dfhack.TranslateName(source.name).." and "..dfhack.TranslateName(partner.name).." are now single")
    end
end

function Marriage (source,target)
    local source_hf = df.historical_figure.find(source.hist_figure_id)
    local target_hf = df.historical_figure.find(target.hist_figure_id)
    source.relationship_ids.Spouse = target.id
    target.relationship_ids.Spouse = source.id

    local new_link = df.histfig_hf_link_spousest:new() -- adding hf link to source
    new_link.target_hf = target_hf.id
    new_link.link_strength = 100
    source_hf.histfig_links:insert('#',new_link)

    new_link = df.histfig_hf_link_spousest:new() -- adding hf link to target
    new_link.target_hf = source_hf.id
    new_link.link_strength = 100
    target_hf.histfig_links:insert('#',new_link)
end

function ChooseNewSpouse (source)

    if not source then
        qerror("no unit") return
    end
    if not dfhack.units.isAdult(source) then
        ErrorPopup("target is too young") return
    end
    if not (source.relationship_ids.Spouse == -1 and source.relationship_ids.Lover == -1) then
        ErrorPopup("target already has a spouse or a lover")
        qerror("source already has a spouse or a lover")
        return
    end

    local choicelist = {}
    targetlist = {}

    for k,v in pairs (df.global.world.units.active) do
        if dfhack.units.isCitizen(v)
            and v.race == source.race
            and v.sex ~= source.sex
            and v.relationship_ids.Spouse == -1
            and v.relationship_ids.Lover == -1
            and dfhack.units.isAdult(v)
            then
                table.insert(choicelist,dfhack.TranslateName(v.name)..', '..dfhack.units.getProfessionName(v))
                table.insert(targetlist,v)
        end
    end

    if #choicelist > 0 then
        ListPrompt(
            "Assign new spouse for "..dfhack.TranslateName(source.name),
            choicelist,
            true,
            function(a,b)
                local target = targetlist[a]
                Marriage (source,target)
                AnnounceAndGamelog(dfhack.TranslateName(source.name).." and "..dfhack.TranslateName(target.name).." have married!")
            end)
    else
        ErrorPopup("No suitable candidates")
    end
end

function MainDialog (source)

    local familystate = GetMarriageSummary(source)

    familystate = familystate.."\nSelect action:"
    local choicelist = {}
    local on_select = {}

    local adult = dfhack.units.isAdult(source)
    local single = source.relationship_ids.Spouse == -1 and source.relationship_ids.Lover == -1
    local ready_for_marriage = single and adult

    if adult then
        table.insert(choicelist,"Remove romantic relationships (if any)")
        table.insert(on_select, Divorce)
        if ready_for_marriage then
            table.insert(choicelist,"Assign a new spouse")
            table.insert(on_select,ChooseNewSpouse)
        end
        if not ready_for_marriage then
            table.insert(choicelist,"[Assign a new spouse]")
            table.insert(on_select,function () ErrorPopup ("Existing relationships must be removed if you wish to assign a new spouse.") end)
        end
    else
        table.insert(choicelist,"Leave this child alone")
        table.insert(on_select,nil)
    end

    ListPrompt(familystate, choicelist, false,
        function(a,b) if on_select[a] then on_select[a](source) end end)
end


local args = {...}

if args[1] == "help" or args[1] == "?" then print(helpstr) return end

if not dfhack.world.isFortressMode() then
    print (helpstr) qerror ("invalid game mode") return
end

if args[1] == "divorce" and tonumber(args[2]) then
    local unit = df.unit.find(tonumber(args[2]))
    if unit then Divorce (unit) return end
end

if tonumber(args[1]) and tonumber(args[2]) then
    local unit1 = df.unit.find(tonumber(args[1]))
    local unit2 = df.unit.find(tonumber(args[2]))
    if unit1 and unit2 then
        Divorce (unit1)
        Divorce (unit2)
        Marriage (unit1,unit2)
        return
    end
end

local selected = dfhack.gui.getSelectedUnit(true)
if tonumber(args[1]) then
    selected = df.unit.find(tonumber(args[1])) or selected
end

if selected then
    if dfhack.units.isCitizen(selected) and dfhack.units.isSane(selected) then
        MainDialog(selected)
    else
        qerror("You must select a sane fortress citizen.")
        return
    end
else
    print (helpstr)
    qerror("Select a sane fortress dwarf")
end
