-- Adds emotions to creatures.
--@ module = true

--[[=begin

add-thought
===========
Adds a thought or emotion to the selected unit.  Can be used by other scripts,
or the gui invoked by running ``add-thought gui`` with a unit selected.

=end]]

local utils=require('utils')

function addEmotionToUnit(unit,thought,emotion,severity,strength,subthought)
    local emotions=unit.status.current_soul.personality.emotions
    if not (type(emotion)=='number') then emotion=df.emotion_type[emotion] end
    if not (type(thought)=='number') then thought=df.unit_thought_type[thought] end
    emotions:insert('#',{new=df.unit_personality.T_emotions,
    type=emotion,
    unk2=1,
    strength=strength,
    thought=thought,
    subthought=subthought,
    severity=severity,
    flags=0,
    unk7=0,
    year=df.global.cur_year,
    year_tick=df.global.cur_year_tick
    })
    local divider=df.emotion_type.attrs[emotion].divider
    if divider~=0 then
        unit.status.current_soul.personality.stress_level=unit.status.current_soul.personality.stress_level+math.ceil(severity/df.emotion_type.attrs[emotion].divider)
    end
end

validArgs = validArgs or utils.invert({
 'unit',
 'thought',
 'emotion',
 'severity',
 'strength',
 'subthought',
 'gui'
})

function tablify(iterableObject)
    t={}
    for k,v in ipairs(iterableObject) do
        t[k] = v~=nil and v or 'nil'
    end
    return t
end

if moduleMode then
 return
end

local args = utils.processArgs({...}, validArgs)

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
          local stok,strength=script.showInputPrompt('emotions','At what strength?',COLOR_WHITE,'0')
          if stok then
            addEmotionToUnit(unit,thought,emotion,severity,strength,0)
          end
        end
      end
    end
  end)
else
    local thought = args.thought or 180

    local emotion = args.emotion or -1

    local severity = args.severity or 0

    local subthought = args.subthought or 0

    local strength = args.strength or 0

    addEmotionToUnit(unit,thought,emotion,severity,strength,subthought)
end
