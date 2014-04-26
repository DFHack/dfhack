-- Lets constructions reconsider the build location.

-- Partial work-around for http://www.bay12games.com/dwarves/mantisbt/view.php?id=5991

local utils = require('utils')

local count = 0

for link,job in utils.listpairs(df.global.world.job_list) do
    local job = link.item
    local place = dfhack.job.getHolder(job)

    if job.job_type == df.job_type.ConstructBuilding
    and place and place:isImpassableAtCreation()
    and job.item_category[0]
    then
        local cpos = utils.getBuildingCenter(place)

        if same_xyz(cpos, job.pos) then
            -- Reset the flag
            job.item_category[0] = false
            job.flags.suspend = false

            -- Mark the tile restricted traffic
            local dsgn,occ = dfhack.maps.getTileFlags(cpos)
            dsgn.traffic = df.tile_traffic.Restricted

            count = count + 1
        end
    end
end

print('Found and unstuck '..count..' construct building jobs.')

if count > 0 then
    df.global.process_jobs = true
end
