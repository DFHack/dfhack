-- Stores lua script data persistently, either globally, or on a per-save basis, without being limited by dfhack.persistent restrictions
-- (I've got no idea how making modules, I'm just copying what I see in other scripts :P)

local _ENV = mkmodule("script-data")
---------------------------------------------------------------------
-- DOCUMENTATION
--[====[

script-data
===========
A service for saving and loading data within lua tables, providing a method of storing data persistently. This is similar to ``persist-table`` as well as the persistence module, but uses DFhack's json implementation directly.

Data can be set to be global (available across every save), or world-based (where each save game can have different information saved). World-based data will automatically be saved whenever the world is exited, and can also be configured to automatically save during autosaves/quicksaves (which is enabled by default).

Global data is stored in ``hack/data/script-data``, while world data is saved within ``[path to save]/script-data``. Data is stored within a json file named after the ID that the table was given by a script.

Main Usage
----------
Storing data is done by placing any information you want to store within the lua table that's returned by ``load_global`` (for global data) or ``load_world`` (for world-based data), which will then be saved to a file when prompted. Both functions take a unique ID key to use as an identifier for that table of data. If there's pre-existing data using that ID (currently already loaded, or previously saved), the table that's returned by the functions will contain that data (in the case that it's already loaded, the table that's returned is the *exact lua table* rather than a copy of it).

You should still use the load functions even if you know that no data by the given ID exists yet, as this is how you get a persistent table to store data under that ID to begin with.

* :load_world(id, do_autosave): Loads world data of the given ID. World data is available only within the currently loaded save. ``do_autosave`` is an optional boolean argument, which governs whether the data should also be saved during autosave/quicksaves. This defaults to true if omitted, and there's unlikely any reason you'd need to disable it.

* :load_global(id): Loads global data of the given ID. Global data is available across all saves. Global data doesn't save automatically, and so should be manually saved with the use of the ``save`` function as necessary.

Data of both global and world types are both saved by the same function:

* :save(id): Triggers the loaded data of the given ID to be saved. This process is done automatically for world type data, but needs to be triggered manually to save global type data.

World data will be saved automatically when the world is unloaded, unless ``quit_without_saving`` has been set to true. ``quit_without_saving`` is set to true automatically during arena mode, as there's no non-hacky way to detect if the player chose to save and quit, or quit without saving.

Notes/Limitations
-----------------
Notes on IDs:

* IDs should be uniquely named. There is overlap between world and global IDs, so if your script uses both they should each be uniquely named (e.g. ``my_script_global`` and ``my_script_world``, rather than both being named ``my_script``).
* Don't use characters that aren't file name friendly, as the IDs are used as the names for their json save files.

There are a few limitations of DFhack's particular json implementation (``jf-json``) to bear in mind:

* You cannot use both string keys and numerical keys within the same table.
* Don't use numerical keys for non-array tables, and don't make sparse arrays, as ``jf-json`` will interpret any table with numbered keys as an array, and attempt to fill in every missing entry when writing to a file. For example, if you were to make a table that only has an entry at index 1000, when it's saved to a file the code will write in additional entries of ``nil`` for all 999 indexes leading up to that entry, bloating the file. If you're using numerical IDs as your keys (like unit IDs, etc.), consider storing them as strings rather than numbers.

Example
-------
This is a simple counter that counts how many times the code has been run in the current save, maintaining its count between sessions. Because it's world data, the value will differ between each save game::

  local my_data = require("script-data").load_world("my_counter_data", true)

  -- Do first time setup if we've never actually stored any data with our ID before
  if next(my_data) == nil then
    my_data.times_run = 0
  end

  my_data.times_run = my_data.times_run + 1

  print("I've run " .. my_data.times_run .. " times in this save!")

  -- Because this is world data and we've enabled do_autosave, we don't have to worry about manually saving, as it'll be saved when the world is unloaded / autosaved automatically!

]====]
---------------------------------------------------------------------
-- VARIABLES
recorded_save_path = recorded_save_path or nil -- Records the save path for the currently loaded world (or last loaded one, for use when saving data after the world has unloaded)
is_saving = is_saving or false -- Records if if the script is currently saving during an autosave
quit_without_saving = quit_without_saving or false -- Flag to allow a save to be unloaded without triggering a save. For now, is just used to automatically prevent saving in arena mode, but could be useful later on if the ability to quit without saving is added to other modes. (only applies to world saves, not globals)

loaded_data = loaded_data or {}
-- Entries are tables, stored under keys of their IDs
--[[ Structure of a loaded data entry:
  id = string -- A unique identifier for the data. It's also used as the TODO
  type = "global" / "world" -- Notes if the data is global, or unique to a world
  autosave = bool -- If true, the data will be automatically saved whenever an autosave / quicksave happens
  data = table -- A table containing the actual data that the script uses, the contents of which will be written to a json file on save.
]]
loop_timer = loop_timer or nil
---------------------------------------------------------------------
-- REQUIRES
local json = require("json")
---------------------------------------------------------------------
-- LOCAL FUNCTIONS
local function create_loaded_data_entry(id, type, autosave)
  local entry = {
    id = id,
    type = type or "global",
    autosave = autosave or false,
    data = {}
  }

  loaded_data[id] = entry
  return entry
end

-- Makes sure the save directory for the script data exists, and if it doesn't make it
local function ensure_save_dir_exists(type)
  local path

  if type == "global" then
    path = dfhack.getHackPath() .. "/data"
  else -- assume world
    -- Ensure recorded_save_path is up to date
    update_save_path()

    path = recorded_save_path
  end

  if not dfhack.filesystem.isdir(path .. "/script-data") then
    local success = dfhack.filesystem.mkdir(path .. "/script-data")

    if not success then
      qerror("Unable to create save directory!")
    end
  end
end

-- Returns the path to
local function get_save_dir(type)
  local path

  if type == "global" then
    path = dfhack.getHackPath() .. "/data"
  else -- assume world
    -- Ensure recorded save path is up to date
    update_save_path()

    path = recorded_save_path
  end

  -- We won't bother with any proper checks here, as running ensure_save_dir_exists will already create any folders that are missing

  return (path .. "/script-data")
end

local function load_data(id, type)
  -- If loaded data with this id already exists, return the persistent table that's already loaded instead of loading from a file
  if get_loaded_data(id) ~= nil then
    return get_loaded_data(id).data
  end

  local path
  local entry

  if type == "world" then
    -- Ensure the world is actually loaded before attempting to load world data
    if not dfhack.isWorldLoaded() then
      return false, "No world is loaded"
    end

    -- Create a loaded data entry, even before trying to load anything from a file
    -- (if there's nothing to load, we'll give the data table from the new entry as the persistent table to use)
    entry = create_loaded_data_entry(id, "world")
    path = get_save_dir("world") .. "/" .. id .. ".json"
  elseif type == "global" then
    entry = create_loaded_data_entry(id, "global")
    path = get_save_dir("global") .. "/" .. id .. ".json"
  end


  -- Load file (if it exists), and update the entry's stored persistent table to contain the loaded data
  if dfhack.filesystem.isfile(path) then
    entry.data = json.decode_file(path)
  end

  return entry.data
end

---------------------------------------------------------------------
-- GLOBAL FUNCTIONS

-- Returns a table whose content is saved into the world save. If there's saved data under the given ID in the world, that data is loaded into the table.
-- Autosave is an optional flag which causes the data to be saved automatically during autosaves/quicksaves if enabled (and is enabled by default)
function load_world(id, autosave)
  local persistent_table = load_data(id, "world")

  -- Set the autosave flag (if provided). Otherwise default to on
  set_autosave(id, autosave or true)

  return persistent_table
end

-- Returns a table whose content is available regardless of currently loaded save. If there's saved data under the given ID, that data is loaded into the table.
function load_global(id)
  local persistent_table = load_data(id, "global")

  return persistent_table
end

-- Trigger the contents of the persistent table of a given ID to be saved to a file.
function save(id)
  -- Trigger presave? TODO

  local path
  if get_loaded_data(id).type == "global" then
    ensure_save_dir_exists("global")
    path = get_save_dir("global") .. "/" .. id .. ".json"
  else -- assume world
    --[[
    -- Safety check for arena mode
    -- Arena mode saves are the only(?) worlds that can trigger world loaded/unloaded without already having an existing save file
    -- DFhack's dfhack.getSavePath() will provide a path for that save game's folder even if it doesn't actually exist!

    -- Check the save folder actually exists before attempting to save - if it doesn't, abort!
    update_save_path()

    if not dfhack.filesystem.isdir(recorded_save_path) then
      return false
    end

    -- Otherwise, continue as usual
    ]]
    --^ Old safety check for before quit_without_saving was added to work around it (the check didn't really work properly anyway)
    ensure_save_dir_exists("world")
    path = get_save_dir("world") .. "/" .. id .. ".json"
  end

  json.encode_file(get_loaded_data(id).data, path)
end

-- Trigger saves for all persistent tables which have the autosave flag currently set
-- This is naturally triggered by this script whenever an autosave / quicksave is detected
function save_autosave()
  for id, entry in pairs(loaded_data) do
    if entry.autosave == true then
      save(id)
    end
  end
end

-- Sets the autosave flag for an entry
function set_autosave(id, do_autosave)
  get_loaded_data(id).autosave = do_autosave
end

-- Set whether the current save should save world entries when quitting
-- This value is reset back to false whenever a world is unloaded / loaded
-- It's automatically set to true whenever an arena game is started
function set_quit_without_saving(bool)
  quit_without_saving = bool
end

-- Returns the loaded data entry (essentially the metadata of a loaded script data) for the script data of the given ID
-- There's not likely a reason to use this externally.
function get_loaded_data(id)
  return loaded_data[id]
end

-- Check to see if save path info is recorded
-- If it's not, attempt to update the record if possible
function update_save_path()
  if recorded_save_path == nil and dfhack.isWorldLoaded() then
    recorded_save_path = dfhack.getSavePath()
  end
end
---------------------------------------------------------------------
-- MISC

local function world_load_setup()
  -- We store the path of the loaded save just in case there's some hiccups with the fact we try to save world data during the SC_WORLD_UNLOADED event (where dfhack.getSavePath() might possibly return nil)
  recorded_save_path = dfhack.getSavePath()

  -- Begin monitoring for autosaves / quicksaves
  -- Since autosaves are only actually relevant in fortress mode, there's no reason to start checking for them outside of that mode
  if dfhack.world.isFortressMode() then
    start_loop()
  end

  -- Automatically disable saving on exit if in arena mode
  -- This is because it's the only current mode(?) that allows for quitting without saving, and there's no way (without some hackery) to really detect that
  -- DFhack will also report it as having a save directory that doesn't actually exist if the arena game hasn't been saved, just to add to the annoyances
  if dfhack.world.isArena() then
    set_quit_without_saving(true)
  else
    set_quit_without_saving(false)
  end
end

dfhack.onStateChange["script-data"] = function(event)
  if event == SC_WORLD_LOADED then
    world_load_setup()

  elseif event == SC_WORLD_UNLOADED then
    -- Trigger the saves for all `world` entries, provided quitting without saving is disabled
    if quit_without_saving == false then
      for id, entry in pairs(loaded_data) do
        if entry.type == "world" then
          save(id)
        end
      end
    end

    -- Now, cleanup world-related data (while still keeping global data around)
    for id, entry in pairs(loaded_data) do
      if entry.type == "world" then
        loaded_data[id] = nil
      end
    end

    recorded_save_path = nil
    set_quit_without_saving(false)

    -- Stop monitoring autosaves
    if loop_timer ~= nil then
      dfhack.timeout_active(loop_timer, nil)
    end
  end
end

function start_loop()
  loop_timer = dfhack.timeout(1, "frames", autosave_check_loop)
end

-- Monitors for when new autosaves begin, triggering appropriate persistent tables to be saved
function autosave_check_loop()
  -- Backup to break the loop if the world is no longer loaded
  if not dfhack.isWorldLoaded() then
    return
  end

  -- Check to see if a new autosave / quicksave is happening
  if df.global.ui.main.autosave_request == true and is_saving == false then
    -- Trigger all persistent tables that should save on autosaves to save:
    save_autosave()
    -- Set a flag so we don't trigger this again on this same autosave:
    is_saving = true
  elseif is_saving == true and df.global.ui.main.autosave_request == false then -- Autosave has finished
    -- Reset the flag for whenever the next autosave happens
    is_saving = false
  end

  -- Restart the loop
  start_loop()
end
---------------------------------------------------------------------
-- INITIAL SETUP

-- It's possible that the first time this is run, a world will have already been loaded up, and so the things that are supposed to have happened during SC_WORLD_LOADED haven't happened, since the event listener wasn't registered at the time
-- As such, perform setup now if necessary:
if dfhack.isWorldLoaded() then
  world_load_setup()
end

---------------------------------------------------------------------
return _ENV
