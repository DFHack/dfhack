-- Improve "Bring up specific incident or rumor" menu in Adventure mode
--@ module = true
--[====[

adv-rumors
==========
Improves the "Bring up specific incident or rumor" menu in Adventure mode.

- Moves entries into one line
- Adds a "slew" keyword for filtering, making it easy to find your kills and not your companions'
- Trims repetitive words

]====]

--========================
-- Author : 1337G4mer on bay12 and reddit
-- Version : 0.2
-- Description : A small utility based on dfhack to improve the rumor UI in adventure mode.
--
--      In game when you want to boast about your kill to someone. Start conversation and choose
--      the menu "Bring up specific incident or rumor"
--      type rumors in dfhack window and hit enter. Or do the below keybind and use that directly from DF window.
--
-- Prior Configuration: (you can skip this if you want)
--      Set the three boolean values below and play around with the script as to how you like
--      improveReadability = will move everything in one line
--      addKeywordSlew = will add a keyword for filtering using slew, making it easy to find your kills and not your companion's
--      shortenString = will further shorten the line to = slew "XYZ" ( "n time" ago in " Region")
--=======================

local utils = require "utils"

local names_blacklist = utils.invert{"a", "an", "you", "attacked", "slew", "was", "slain", "by"}

function condenseChoiceTitle(choice)
    while #choice.title > 1 do
        choice.title[0].value = choice.title[0].value .. ' ' .. choice.title[1].value
        choice.title:erase(1)
    end
end

function addKeyword(choice, keyword)
    local keyword_ptr = df.new('string')
    keyword_ptr.value = keyword
    choice.keywords:insert('#', keyword_ptr)
end

function rumorUpdate()
    local improveReadability = true
    local addKeywordSlew = true
    local shortenString = true
    local addKeywordNames = true

    for i, choice in ipairs(df.global.adventure.conversation.choices) do
        if choice.choice.type == df.talk_choice_type.SummarizeConflict then
            if improveReadability then
                condenseChoiceTitle(choice)
            end
            if shortenString then
                condenseChoiceTitle(choice)
                choice.title[0].value = choice.title[0].value
                    :gsub("Summarize the conflict in which +", "")
                    :gsub("This occurred +", "")
            end
            if addKeywordSlew then
                if string.find(choice.title[0].value, "slew") then
                    addKeyword(choice, 'slew')
                end
            end
            if addKeywordNames then
                local title = choice.title[0].value
                for keyword in title:sub(1, title:find('%(') - 1):gmatch('%w+') do
                    keyword = dfhack.utf2df(dfhack.df2utf(keyword):lower())
                    if not names_blacklist[keyword] then
                        addKeyword(choice, keyword)
                    end
                end
            end
        end
    end
end

rumorUpdate()
