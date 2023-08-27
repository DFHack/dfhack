-- Create a tree at the cursor position.
-- Intended to act as a user-friendly front for modtools/create-tree
-- Author: Atomic Chicken

--[====[

gui/create-tree
===============
A graphical interface for creating trees.

Place the cursor wherever you want the tree to appear and run the script.
Then select the desired tree type from the list.
You will then be asked to input the desired age of the tree in years.
If omitted, the age will default to 1.

]====]

function spawnTree()
  local dialogs = require 'gui.dialogs'
  local createTree = reqscript('modtools/create-tree')

  local x, y, z = pos2xyz(df.global.cursor)
  if not x then
    qerror("First select a spawn location using the cursor.")
  end
  local treePos = {x, y, z}

  local treeRaws = {}
  for _, treeRaw in ipairs(df.global.world.raws.plants.trees) do
    table.insert(treeRaws, {text = treeRaw.name, treeRaw = treeRaw, search_key = treeRaw.name:lower()})
  end
  dialogs.showListPrompt('Spawn Tree', 'Select a plant:', COLOR_LIGHTGREEN, treeRaws, function(id, choice)
    local treeAge
    dialogs.showInputPrompt('Age', 'Enter the age of the tree (in years):', COLOR_LIGHTGREEN, nil, function(input)
      if not input then
        return
      elseif input == '' then
        treeAge = 1
      elseif tonumber(input) and tonumber(input) >= 0 then
        treeAge = tonumber(input)
      else
        dialogs.showMessage('Error', 'Invalid age: ' .. input, COLOR_LIGHTRED)
      end
      createTree.createTree(choice.treeRaw, treeAge, treePos)
    end)
  end, nil, nil, true)
end

spawnTree()
