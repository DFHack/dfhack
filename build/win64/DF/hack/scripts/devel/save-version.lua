-- Display DF version information about the current save
--@module = true
--[====[

devel/save-version
==================
Display DF version information about the current save

]====]

local function dummy() return nil end

function has_field(tbl, field)
    for k in pairs(tbl) do
        if k == field then
            return true
        end
    end
    return false
end

--luacheck: global
versions = {
-- skipped v0.21-v0.28
    [1287] = "0.31.01",
    [1288] = "0.31.02",
    [1289] = "0.31.03",
    [1292] = "0.31.04",
    [1295] = "0.31.05",
    [1297] = "0.31.06",
    [1300] = "0.31.08",
    [1304] = "0.31.09",
    [1305] = "0.31.10",
    [1310] = "0.31.11",
    [1311] = "0.31.12",
    [1323] = "0.31.13",
    [1325] = "0.31.14",
    [1326] = "0.31.15",
    [1327] = "0.31.16",
    [1340] = "0.31.17",
    [1341] = "0.31.18",
    [1351] = "0.31.19",
    [1353] = "0.31.20",
    [1354] = "0.31.21",
    [1359] = "0.31.22",
    [1360] = "0.31.23",
    [1361] = "0.31.24",
    [1362] = "0.31.25",

    [1372] = "0.34.01",
    [1374] = "0.34.02",
    [1376] = "0.34.03",
    [1377] = "0.34.04",
    [1378] = "0.34.05",
    [1382] = "0.34.06",
    [1383] = "0.34.07",
    [1400] = "0.34.08",
    [1402] = "0.34.09",
    [1403] = "0.34.10",
    [1404] = "0.34.11",

    [1441] = "0.40.01",
    [1442] = "0.40.02",
    [1443] = "0.40.03",
    [1444] = "0.40.04",
    [1445] = "0.40.05",
    [1446] = "0.40.06",
    [1448] = "0.40.07",
    [1449] = "0.40.08",
    [1451] = "0.40.09",
    [1452] = "0.40.10",
    [1456] = "0.40.11",
    [1459] = "0.40.12",
    [1462] = "0.40.13",
    [1469] = "0.40.14",
    [1470] = "0.40.15",
    [1471] = "0.40.16",
    [1472] = "0.40.17",
    [1473] = "0.40.18",
    [1474] = "0.40.19",
    [1477] = "0.40.20",
    [1478] = "0.40.21",
    [1479] = "0.40.22",
    [1480] = "0.40.23",
    [1481] = "0.40.24",

    [1531] = "0.42.01",
    [1532] = "0.42.02",
    [1533] = "0.42.03",
    [1534] = "0.42.04",
    [1537] = "0.42.05",
    [1542] = "0.42.06",

    [1551] = "0.43.01",
    [1552] = "0.43.02",
    [1553] = "0.43.03",
    [1555] = "0.43.04",
    [1556] = "0.43.05",

    [1596] = "0.44.01",
    [1597] = "0.44.02",
    [1600] = "0.44.03",
    [1603] = "0.44.04",
    [1604] = "0.44.05",
    [1611] = "0.44.06",
    [1612] = "0.44.07",
    [1613] = "0.44.08",
    [1614] = "0.44.09",
    [1620] = "0.44.10",
    [1623] = "0.44.11",
    [1625] = "0.44.12",

    [1710] = "0.47.01",
    [1711] = "0.47.02",
    [1713] = "0.47.03",
    [1715] = "0.47.04",
    [1716] = "0.47.05",

    [2078] = "50.01",
    [2079] = "50.02",
    [2080] = "50.03",  -- and 50.04
    [2081] = "50.05",  -- through 50.09
}

--luacheck: global
min_version = math.huge
--luacheck: global
max_version = -math.huge

for k in pairs(versions) do
    min_version = math.min(min_version, k)
    max_version = math.max(max_version, k)
end

if has_field(df.global.world, 'save_version') then
    function get_save_version()
        return df.global.world.save_version
    end
elseif df.world.T_cur_savegame and has_field(df.global.world.cur_savegame, 'save_version') then
    function get_save_version()
        return df.global.world.cur_savegame.save_version
    end
elseif has_field(df.global.world.pathfinder, 'anon_2') then
    function get_save_version()
        return df.global.world.pathfinder.anon_2
    end
else
    get_save_version = dummy
end

if has_field(df.global.world, 'original_save_version') then
    function get_original_save_version()
        return df.global.world.original_save_version
    end
else
    get_original_save_version = dummy
end

function describe(version)
    if version == 0 then
        return 'not saved'
    elseif versions[version] then
        return versions[version] .. (' (%i)'):format(version)
    elseif version < min_version then
        return 'unknown old version before ' .. describe(min_version) .. ': ' .. tostring(version)
    elseif version > max_version then
        return 'unknown new version after ' .. describe(max_version) .. ': ' .. tostring(version)
    else
        return 'unknown version: ' .. tostring(version)
    end
end

function dump(desc, func)
    local ret = tonumber(func())
    if ret then
        print(('%-25s %s'):format(desc .. ':', describe(ret)))
    else
        dfhack.printerr('could not find ' .. desc .. ' (DFHack version too old)')
    end
end

if not moduleMode then
    if not dfhack.isWorldLoaded() then qerror('no world loaded') end
    dump('original DF version', get_original_save_version)
    dump('most recent DF version', get_save_version)
    dump('running DF version', function() return df.global.version end)
end
