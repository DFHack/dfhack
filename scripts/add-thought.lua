-- Adds thoughts to creatures. Use add-thought -help for more information.
-- author Putnam

local function addEmotionToUnit(emotions,thought,emotion,severity)
    if not (type(emotion)=='number') then emotion=df.emotion_type[emotion] end
    if not (type(thought)=='number') then thought=df.unit_thought_type[thought] end
    emotions:insert('#',{new=df.unit_personality.T_emotions,
    type=emotion,
    unk2=1,
    unk3=1,
    thought=thought,
    subthought=0,
    severity=severity,
    flags=0,
    unk7=0,
    year=df.global.cur_year,
    year_tick=df.global.cur_year_tick
    })
end

function tablify(iterableObject)
    t={}
    for k,v in ipairs(iterableObject) do
        t[k] = v~=nil and v or 'nil'
    end
    return t
end


local utils=require('utils')

validArgs = validArgs or utils.invert({
 'unit',
 'thought',
 'emotion',
 'severity',
 'gui',
 'help'
})

local args = utils.processArgs({...}, validArgs)

if args.help then
    print(' add-thought: add a thought to a unit with an associated emotion.')
    print('  add-thought -gui: opens a gui to add a thought to the selected unit.')
    print('  add-thought -emotion numOrName -thought numOrName -severity num')
    print('   adds thought with given thought, emotion and severity to selected unit.')
    print('   names can be found here:')
    print('   https://github.com/DFHack/df-structures/blob/master/df.unit-thoughts.xml')
    print('  add-thought -unit etc.: as two above, but instead of selected unit uses unit')
    print('  with given ID. (for use with modtools)')
end

local unit = args.unit and df.unit.find(args.unit) or dfhack.gui.getSelectedUnit(true)

if not unit then qerror('A unit must be specified or selected.') end
if args.gui then
    local script=require('gui.script')
    script.start(function()
    local tok,thought=script.showListPrompt('emotions','Which thought?',COLOR_WHITE,tablify(df.unit_thought_type),10,true)
     if tok then
      local eok,emotion=script.showListPrompt('emotions','Which emotion?',COLOR_WHITE,tablify(df.emotion_type),10,true)
       if eok then
        local sok,severity=script.showInputPrompt('emotions','At what severity?',COLOR_WHITE,'0')
         if sok then
          addEmotionToUnit(unit.status.current_soul.personality.emotions,thought,emotion,severity)
         end
       end
     end
    end)
else
    local thought = args.thought or 180
    
    local emotion = args.emotion or -1
    
    local severity = args.severity or 0
    
    addEmotionToUnit(unit.status.current_soul.personality.emotions,thought,emotion,severity)
end