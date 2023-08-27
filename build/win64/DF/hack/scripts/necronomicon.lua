-- Author: Ajhaa

-- lists books that contain secrets of life and death
local utils = require("utils")
local argparse = require("argparse")


function get_book_interactions(item)
    local book_interactions = {}
    for _, improvement in ipairs(item.improvements) do
        if improvement._type == df.itemimprovement_pagesst or
           improvement._type == df.itemimprovement_writingst then
            for _, content_id in ipairs(improvement.contents) do
                local written_content = df.written_content.find(content_id)

                for _, ref in ipairs (written_content.refs) do
                    if ref._type == df.general_ref_interactionst then
                        local interaction = df.global.world.raws.interactions[ref.interaction_id]
                        table.insert(book_interactions, interaction)
                    end
                end
            end
        end
    end

    return book_interactions
end

function check_slab_secrets(item)
    local type_id = item.engraving_type
    local type = df.slab_engraving_type[type_id]
    return type == "Secrets"
end

function get_item_artifact(item)
    for _, ref in ipairs(item.general_refs) do
        if ref._type == df.general_ref_is_artifactst then
            return df.global.world.artifacts.all[ref.artifact_id]
        end
    end
end

function print_interactions(interactions)
    for _, interaction in ipairs(interactions) do
        -- Search interaction.str for the tag [CDI:ADV_NAME:<string>]
        -- for example: [CDI:ADV_NAME:Raise fetid corpse]
        for _, str in ipairs(interaction.str) do
            local _, e = string.find(str.value, "ADV_NAME")
            if e then
                print("\t", string.sub(str.value, e + 2, #str.value - 1))
            end
        end
    end
end

function necronomicon(include_slabs)
    if include_slabs then
        print("SLABS:")
        for _, item in ipairs(df.global.world.items.other.SLAB) do
            if check_slab_secrets(item) then
                local artifact = get_item_artifact(item)
                local name = dfhack.TranslateName(artifact.name)
                print(dfhack.df2console(name))
            end
        end
        print()
    end
    print("BOOKS:")
    for _, item in ipairs(df.global.world.items.other.BOOK) do
        local interactions = get_book_interactions(item)

        if next(interactions) ~= nil then
            print(item.title)
            print_interactions(interactions)
        end
    end
end


local help = false
local include_slabs = false
local args = argparse.processArgsGetopt({...}, {
    {"s", "include-slabs", handler=function() include_slabs = true end},
    {"h", "help", handler=function() help = true end}
})

local cmd = args[1]

if help or cmd == "help" then
    print(dfhack.script_help())
elseif cmd == nil or cmd == "" then
    necronomicon(include_slabs)
else
    print("necronomicon: Invalid argument \"" .. cmd .. "\"")
end
