-- Adds emotions to creatures.

local utils=require('utils')

local function addEmotionToUnit(emotions,thought,emotion,severity,subthought)
    if not (type(emotion)=='number') then emotion=df.emotion_type[emotion] end
    if not (type(thought)=='number') then thought=df.unit_thought_type[thought] end
    emotions:insert('#',{new=df.unit_personality.T_emotions,
    type=emotion,
    unk2=1,
    strength=1,
    thought=thought,
    subthought=subthought,
    severity=severity,
    flags=0,
    unk7=0,
    year=df.global.cur_year,
    year_tick=df.global.cur_year_tick
    })
end

validArgs = validArgs or utils.invert({
 'unit',
 'thought',
 'emotion',
 'severity',
 'gui'
})

function tablify(iterableObject)
    t={}
    for k,v in ipairs(iterableObject) do
        t[k] = v~=nil and v or 'nil'
    end
    return t
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
                    addEmotionToUnit(unit.status.current_soul.personality.emotions,thought,emotion,severity,0)
                end
            end
        end
    end)
else
    local thought = args.thought or 180
    
    local emotion = args.emotion or -1
    
    local severity = args.severity or 0
    
    local subthought = args.subthought or 0
    
    addEmotionToUnit(unit.status.current_soul.personality.emotions,thought,emotion,severity,subthought)
end
