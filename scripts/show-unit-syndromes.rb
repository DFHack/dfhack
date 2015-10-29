# Show syndromes affecting units and the remaining and maximum duration
# original author: drayath
# edited by expwnent
=begin

show-unit-syndromes
===================
Show syndromes affecting units and the remaining and maximum duration, along
with (optionally) substantial detail on the effects.

Use one or more of the following options:

:help:                  Show the help message
:showall:               Show units even if not affected by any syndrome
:showeffects:           Show detailed effects of each syndrome
:showdisplayeffects:    Show effects that only change the look of the unit
:selected:              Show selected unit
:dwarves:               Show dwarves
:livestock:             Show livestock
:wildanimals:           Show wild animals
:hostile:               Show hostiles (e.g. invaders, thieves, forgotten beasts etc)
:world:                 Show all defined syndromes in the world
:export:                ``export:<filename>`` sends output to the given file, showing all
                        syndromes affecting each unit with the maximum and present duration.

=end

#TODO: When showing effects on a unit, show the actual change to the unit
#       E.g. if +150%, +500 strength show actual total bonus based on the unit stats.
# For this also need to know
#        how does size_delays affect the start/peak/end time
#        how does size_dilute affect the Severity, does it also affect the phy/mental stat adjustments?
#        how does peak affect the Severity, does it also affect the phy/mental stat adjustments?
#TODO: Add interaction info needs to display a bit more data, but the required structures are not yet decoded

#TODO: Several of the unk_xxx fields have been identified here, and can get some more by comparing the raws with the printed interaction and effect information. Pass these onto the dfhack guys.

def print_help()
  puts "Use one or more of the following options:"
  puts "    showall: Show units even if not affected by any syndrome"
  puts "    showeffects: shows detailed effects of each syndrome"
  puts "    showdisplayeffects: show effects that only change the look of the unit"
  puts "    ignorehiddencurse: Hides syndromes the user should not be able to know about (TODO)"
  puts "    selected: Show selected unit"
  puts "    dwarves: Show dwarves"
  puts "    livestock: Show livestock"
  puts "    wildanimals: Show wild animals"
  puts "    hostile: Show hostiles (e.g. invaders, thieves, forgotten beasts etc)"
  puts "    world: Show all defined syndromes in the world"
  puts "    export:<filename> Write the output to a file instead of the console."
  puts ""
  puts "Will show all syndromes affecting each units with the maximum and present duration."
end

class Output
  attr_accessor :fileLogger, :indent_level

  def initialize(filename)
    indent_level = ""
    if filename==nil
      @fileLogger = nil
    else
      @fileLogger = File.new(filename + ".html", "w")
      @fileLogger.puts("<html><body>")
    end
  end

  RED = "red"
  GREEN = "green"
  BLUE = "blue"
  DEFAULT = "black"
  HIGHLIGHT = "black\" size=\"+1"

  def colorize(text, color_code)
    if @fileLogger == nil
      return text
    else
      new_text = "<font color=\"#{color_code}\">#{text}</font>"
      if color_code == HIGHLIGHT
        new_text = "<b>" + new_text + "</b>"
      end

      return new_text
    end
  end

  def inactive(text)
    if @fileLogger == nil
      return "###" + text
    else
      return "<del>#{text}</del>"
    end
  end

  def indent()
    if @fileLogger == nil
      @indent_level = "#{@indent_level} - "
    else
      @fileLogger.puts("<ul>")
    end
  end

  def unindent()
    if @fileLogger == nil
      @indent_level = @indent_level.chomp(" - ")
    else
      @fileLogger.puts("</ul>")
    end
  end

  def break()
    if @fileLogger == nil
      puts("\n")
    else
      @fileLogger.puts("<hr/></br>")
    end
  end

  def close()
    if @fileLogger != nil
      @fileLogger.puts("</body></html>")
      @fileLogger.flush
      @fileLogger.close
      @fileLogger = nil
    end
  end

  def log(text, color=nil)
    if @fileLogger == nil
      puts("#{@indent_level}#{text}")
    elsif color==nil
      @fileLogger.puts(text+"<br/>")
    elsif @indent_level == ""
      @fileLogger.puts(colorize(text, color))
    else
      @fileLogger.puts("<li>" + colorize(text, color)+"</li>")
    end
  end
end

def get_mental_att(att_index)

  case att_index
  when 0
    return "Analytical Ability"
  when 1
    return "Focus"
  when 2
    return "Willpower"
  when 3
    return "Creativity"
  when 4
    return "Intuition"
  when 5
    return "Patience"
  when 6
    return "Memory"
  when 7
    return "Linguistics"
  when 8
    return "Spacial Sense"
  when 9
    return "Musicality"
  when 10
    return "Kinesthetic Sense"
  when 11
    return "Empathy"
  when 12
    return "Social Awareness"
  else
    return "Unknown"
  end
end

def get_physical_att(att_index)

  case att_index
  when 0
    return "strength"
  when 1
    return "agility"
  when 2
    return "toughness"
  when 3
    return "endurance"
  when 4
    return "recuperation"
  when 5
    return "disease resistance"
  else
    return "unknown"
  end
end

def get_effect_target(target)

  values = []

  limit = target.key.length - 1
  for i in (0..limit)

    if(target.mode[i].to_s() != "")

      items = ""

      #case target.mode[i].to_s()
      #when "BY_TYPE"
      #  item = "type("
      #when "BY_TOKEN"
      #  item = "token("
      #when "BY_CATEGORY"
      #  item = "category("
      #end

      if(target.key[i].to_s()!="")
        item = "#{item}#{target.key[i].to_s().capitalize()}"
      end

      if target.tissue[i].to_s() != "ALL"
        if(target.key[i].to_s()!="" and target.tissue[i].to_s()!="")
          item = "#{item}:"
        end

        if(target.tissue[i].to_s()!="")
          item = "#{item}#{target.tissue[i].to_s().capitalize()}"
        end
      end

      #item = item + ")"

      values.push(item)
    end

  end

  if values.length == 0 or (values.length == 1 and values[0] == "All")
    return ""
  else
    return ", target=" + values.join(", ")
  end
end

def get_att_pairs(values, percents, physical)

  items = []

  color = Output::DEFAULT

  limit = values.length - 1
  for i in (0..limit)
    if (values[i] != 0 or percents[i] != 100)

      if physical
        item = "#{get_physical_att(i)}("
      else
        item = "#{get_mental_att(i)}("
      end

      if(values[i]!=0)
        item = item + "%+d" % values[i]
      end

      if (values[i]!=0 and percents[i]!=100)
        item = item + ", "
      end

      if (percents[i]!=100 or values[i]==0)
        item = item + "%d" % percents[i] + "%"
      end

      item = item + ")"

      if color != Output::RED and values[i] >= 0 and percents[i] > 100
        color = Output::GREEN
      elsif values[i] <0 || percents[i] < 100
        color = Output::RED
      end

      items.push(item)
    end
  end

  return items.join(", "), color
end

def get_display_name(name, verb)
  if name != nil and name != ""
    return name.capitalize()
  end

  if verb == nil or verb == ""
    return "Mystery"
  end

  if verb.length > 100
    verb = verb.slice(0, 100).capitalize()
  end

  pos = verb.index(".")
  if pos == nil
    return verb.slice(0, verb.rindex(" ")).capitalize()
  else
    return verb.slice(0, pos).capitalize()
  end
end

def get_interaction(interaction)

  # name, USAGE_HINT, range, wait period are probably all we really want to show.

  #result = "a=#{interaction.unk_6c} b=#{interaction.unk_7c} c=#{interaction.unk_8c} d=#{interaction.unk_a8} e=#{interaction.unk_c4} f=#{interaction.unk_e4} "
  #result = result + "g=#{interaction.unk_e0} h=#{interaction.unk_e4} i=#{interaction.unk_100} j=#{interaction.unk_11c} k=#{interaction.unk_138} l=#{interaction.unk_154} "
  #result = result + "m=#{interaction.unk_170} n=#{interaction.unk_18c} o=#{interaction.unk_1a8} p=#{interaction.unk_1c4} q=#{interaction.unk_1e8} r=#{interaction.unk_25c} "
  #result = result + "s=#{interaction.unk_278}"

  if interaction.name == ""
    name = "mystery"
  else
    name = interaction.name
  end

  return "ability=#{get_display_name(interaction.name, interaction.verb[0])}, delay=#{interaction.usage_delay}, actionType=TODO, range=TODO, maxTargets=TODO"
end

def get_effect_flags(flags)

  values = []

  if(flags.SIZE_DELAYS) then values.push("size delays") end
  if(flags.SIZE_DILUTES) then values.push("size dilutes") end
  if(flags.VASCULAR_ONLY) then values.push("vascular only") end
  if(flags.MUSCULAR_ONLY) then values.push("muscles only") end
  if(flags.RESISTABLE) then values.push("resistable") end
  if(flags.LOCALIZED) then values.push("localized") end

  return values.join(",")
end

def get_tag1_flags(logger, flags, add)

  values = []

  good = false
  bad = false

  if add
    good_color = Output::GREEN
    bad_color = Output::RED
  else
    good_color = Output::RED
    bad_color = Output::GREEN
  end

  if(flags.EXTRAVISION)
    values.push(logger.colorize("extravision", good_color))
    good = true
  end

  if(flags.OPPOSED_TO_LIFE)
    values.push(logger.colorize("attack the living", bad_color))
    bad = true
  end

  if(flags.NOT_LIVING)
    values.push(logger.colorize("undead", Output::DEFAULT))
  end

  if(flags.NOEXERT)
    values.push(logger.colorize("does not tire", good_color))
    good = true
  end

  if(flags.NOPAIN)
    values.push(logger.colorize("does not feel pain", good_color))
    good = true
  end

  if(flags.NOBREATHE)
    values.push(logger.colorize("does not breathe", good_color))
    good = true
  end

  if(flags.HAS_BLOOD)
    values.push(logger.colorize("has blood", Output::DEFAULT))
  end

  if(flags.NOSTUN)
    values.push(logger.colorize("can't be stunned", good_color))
    good = true
  end

  if(flags.NONAUSEA)
    values.push(logger.colorize("does not get nausea", good_color))
    good = true
  end

  if(flags.NO_DIZZINESS)
    values.push(logger.colorize("does not get dizzy", good_color))
    good = true
  end

  if(flags.NO_FEVERS)
    values.push(logger.colorize("does not get fever", good_color))
    good = true
  end

  if(flags.TRANCES)
    values.push(logger.colorize("can enter trance", good_color))
    good = true
  end

  if(flags.NOEMOTION)
    values.push(logger.colorize("feels no emotion", good_color))
    good = true
  end

  if(flags.LIKES_FIGHTING)
    values.push(logger.colorize("like fighting", Output::DEFAULT))
  end

  if(flags.PARALYZEIMMUNE)
    values.push(logger.colorize("can't be paralyzed", good_color))
    good = true
  end
  if(flags.NOFEAR)
    values.push(logger.colorize("does not feel fear", good_color))
    good = true
  end

  if(flags.NO_EAT)
    values.push(logger.colorize("does not eat", good_color))
    good = true
  end

  if(flags.NO_DRINK)
    values.push(logger.colorize("does not drink", good_color))
    good = true
  end

  if(flags.NO_SLEEP)
    values.push(logger.colorize("does not sleep", good_color))
    good = true
  end
  if(flags.MISCHIEVOUS)
    values.push(logger.colorize("mischievous", Output::DEFAULT))
  end

  if(flags.NO_PHYS_ATT_GAIN)
    values.push(logger.colorize("physical stats cant improve", good_color))
    good = true
  end

  if(flags.NO_PHYS_ATT_RUST)
    values.push(logger.colorize("physical stats do not rust", good_color))
    good = true
  end

  if(flags.NOTHOUGHT)
    values.push(logger.colorize("stupid", bad_color))
    bad = true
  end

  if(flags.NO_THOUGHT_CENTER_FOR_MOVEMENT)
    values.push(logger.colorize("no brain needed to move", good_color))
    good = true
  end

  if(flags.CAN_SPEAK)
    values.push(logger.colorize("can speak", good_color))
    good = true
  end

  if(flags.CAN_LEARN)
    values.push(logger.colorize("can learn", good_color))
    good = true
  end

  if(flags.UTTERANCES)
    values.push(logger.colorize("utterances", Output::DEFAULT))
  end

  if(flags.CRAZED)
    values.push(logger.colorize("crazed", bad_color))
    bad = true
  end

  if(flags.BLOODSUCKER)
    values.push(logger.colorize("drinks blood", bad_color))
    bad = true
  end

  if(flags.NO_CONNECTIONS_FOR_MOVEMENT)
    values.push(logger.colorize("can move without nerves", good_color))
    good = true
  end

  if(flags.SUPERNATURAL)
    values.push(logger.colorize("supernatural", good_color))
    good = true
  end

  if add
    if bad
      color = Output::RED
    elsif good
      color = Output::GREEN
    else
      color = Output::DEFAULT
    end
  else
    if good
      color = Output::RED
    elsif bad
      color = Output::GREEN
    else
      color = Output::DEFAULT
    end
  end

  return values.join(", "), color
end

def get_tag2_flags(logger, flags, add)
  values = []

  good = false
  bad = false

  if add
    good_color = Output::GREEN
    bad_color = Output::RED
  else
    good_color = Output::RED
    bad_color = Output::GREEN
  end

  if(flags.NO_AGING)
    good = true
    values.push(logger.colorize("does not age", good_color))
  end

  if(flags.MORTAL)
    bad = true
    values.push(logger.colorize("mortal", bad_color))
  end

  if(flags.STERILE)
    values.push(logger.colorize("can't have children", Output::DEFAULT))
  end

  if(flags.FIT_FOR_ANIMATION)
    values.push(logger.colorize("can be animated", Output::DEFAULT))
  end

  if(flags.FIT_FOR_RESURRECTION)
    good = true
    values.push(logger.colorize("can be resurrected", Output::DEFAULT))
  end

  if add
    if bad
      color = Output::RED
    elsif good
      color = Output::GREEN
    else
      color = Output::DEFAULT
    end
  else
    if good
      color = Output::RED
    elsif bad
      color = Output::GREEN
    else
      color = Output::DEFAULT
    end
  end

  return values.join(", "), color
end

def find_creature_name(id, casteid)
  creature = df.world.raws.creatures.all.find{ |c| c.creature_id == id }

  if creature == nil
    return id, casteid
  end

  creature_name = creature.name[0].capitalize()

  if casteid == "DEFAULT"
    return creature_name, ""
  end

 caste = creature.caste.find{ |c| c.caste_id == casteid }

  if caste == nil
    return creature_name, casteid
  elsif creature.name[0].downcase() == caste.caste_name[0].downcase()
    return creature_name, ""
  else
    castename = caste.caste_name[0].downcase().chomp(creature.name[0].downcase()).strip()

    if castename.start_with?(creature.name[0])
      castename = castename.slice(creature.name[0].length, castename.length - creature.name[0].length).strip()
    end

    if castename.start_with?("of the")
      castename = castename.slice("of the".length, castename.length - "of the".length).strip()
    end

    return creature_name, castename.downcase()
  end
end

def get_effect(logger, ce, ticks, showdisplayeffects)

  flags = get_effect_flags(ce.flags)
  if flags != ""
    flags = " (#{flags})"
  end

  if ce.end == -1
    duration = " [permanent]"
  elsif ce.start >= ce.peak or ce.peak <= 1
    duration = " [#{ce.start}-#{ce.end}]"
  else
    duration = " [#{ce.start}-#{ce.peak}-#{ce.end}]"
  end

  case ce.getType().to_s()
  when "PAIN"
    name = "Pain"
    desc = "power=#{ce.sev}#{get_effect_target(ce.target)}"
    color = Output::RED
  when "SWELLING"
    name = "Swelling"
    desc = "power=#{ce.sev}#{get_effect_target(ce.target)}"
    color = Output::RED
  when "OOZING"
    name = "Oozing"
    desc = "power=#{ce.sev}#{get_effect_target(ce.target)}"
    color = Output::RED
  when "BRUISING"
    name = "Bruising"
    desc = "power=#{ce.sev}#{get_effect_target(ce.target)}"
    color = Output::RED
  when "BLISTERS"
    name = "Blisters"
    desc = "power=#{ce.sev}#{get_effect_target(ce.target)}"
    color = Output::RED
  when "NUMBNESS"
    name = "Numbness"
    desc = "power=#{ce.sev}#{get_effect_target(ce.target)}"
    color = Output::GREEN
  when "PARALYSIS"
    name = "Paralysis"
    desc = "power=#{ce.sev}#{get_effect_target(ce.target)}"
    color = Output::RED
  when "FEVER"
    name = "Fever"
    desc = "power=#{ce.sev}"
    color = Output::RED
  when "BLEEDING"
    name = "Bleeding"
    desc = "power=#{ce.sev}#{get_effect_target(ce.target)}"
    color = Output::RED
  when "COUGH_BLOOD"
    name = "Cough Blood"
    desc = "power=#{ce.sev}"
    color = Output::RED
  when "VOMIT_BLOOD"
    name = "Vomit Blood"
    desc = "power=#{ce.sev}"
    color = Output::RED
  when "NAUSEA"
    name = "Nausea"
    desc = "power=#{ce.sev}"
    color = Output::RED
  when "UNCONSCIOUSNESS"
    name = "Unconsciousness"
    desc = "power=#{ce.sev}"
    color = Output::RED
  when "NECROSIS"
    name = "Necrosis"
    desc = "power=#{ce.sev}#{get_effect_target(ce.target)}"
    color = Output::RED
  when "IMPAIR_FUNCTION"
    name = "Impairs"
    desc = "power=#{ce.sev}#{get_effect_target(ce.target)}"
    color = Output::RED
  when "DROWSINESS"
    name = "Drowsiness"
    desc = "power=#{ce.sev}"
    color = Output::RED
  when "DIZZINESS"
    name = "Dizziness"
    desc = "power=#{ce.sev}"
    color = Output::RED
  when "ADD_TAG"
    name = "Add"
    tags1 = get_tag1_flags(logger, ce.tags1, true)
    tags2 = get_tag2_flags(logger, ce.tags2, true)
    desc = "#{tags1[0]},#{tags2[0]}"

    if tags1[1] == Output::RED || tags2[1] == Output::RED
      color = Output::RED
    elsif tags1[1] == Output::GREEN || tags2[1] == Output::GREEN
      color = Output::GREEN
    else
      color = Output::DEFAULT
    end
  when "REMOVE_TAG"
    name = "Remove"
    tags1 = get_tag1_flags(logger, ce.tags1, true)
    tags2 = get_tag2_flags(logger, ce.tags2, true)
    desc = "#{tags1[0]},#{tags2[0]}"

    if tags1[1] == Output::RED || tags2[1] == Output::RED
      color = Output::RED
    elsif tags1[1] == Output::GREEN || tags2[1] == Output::GREEN
      color = Output::GREEN
    else
      color = Output::DEFAULT
    end
  when "DISPLAY_TILE"
    if !showdisplayeffects then return "", Output::DEFAULT end
    name = "Tile"
    desc = "Tile=#{ce.tile}, Colour=#{ce.color}"
    color = Output::DEFAULT
  when "FLASH_TILE"
    if !showdisplayeffects then return "", Output::DEFAULT end
    name = "Flash"
    color = ce.sym_color >> 8
    tile = ce.sym_color - (color * 256)
    desc = "tile = #{tile}, colour=#{color}, time=#{ce.period}, period=#{ce.time}"
    color = Output::DEFAULT
  when "SPEED_CHANGE"
    name = "Physical"
    desc = "speed("

    value = ce.bonus_add
    percent = ce.bonus_perc
    if(value!=0)
      desc = desc + "%+d" % value
    end

    if (value!=0 and percent!=100)
      desc = desc + ", "
    end

    if (percent!=100 or value==0)
      desc = desc + "%d" % percent + "%"
    end

    desc = desc + ")"

    if value < 0 or percent < 100
      color = Output::RED
    elsif value >0 or percent >100
      color = Output::GREEN
    else
      color = Output::DEFAULT
    end

  when "CAN_DO_INTERACTION"
    name = "Add interaction"
    desc = "#{get_interaction(ce)}"
    color = Output::GREEN
  when "SKILL_ROLL_ADJUST"
    name = "Skill check"
    desc = "modifier=#{ce.multiplier}%, chance=#{ce.chance}%"

    if ce.multiplier > 100
      color = Output::GREEN
    elsif ce.multiplier < 100
      color = Output::RED
    else
      color = Output::DEFAULT
    end

  when "BODY_TRANSFORMATION"
    name = "Transformation"

    if ce.chance > 0
      chance = ", chance=#{ce.chance} "
    else
      chance = ""
    end

    creature_name = find_creature_name(ce.race_str, ce.caste_str)

    if creature_name[1] == ""
        desc = "#{creature_name[0]}#{chance}"
    else
        desc = "#{creature_name[0]}(#{creature_name[1]})#{chance}"
    end

    color = Output::BLUE
  when "PHYS_ATT_CHANGE"
    name = "Physical"
    data = get_att_pairs(ce.phys_att_add, ce.phys_att_perc, true)
    desc = data[0]
    color = data[1]
  when "MENT_ATT_CHANGE"
    name = "Mental"
    data = get_att_pairs(ce.ment_att_add, ce.ment_att_perc, false)
    desc = data[0]
    color = data[1]
  when "MATERIAL_FORCE_MULTIPLIER"
    name = "Material force multiplier"
    desc = "received damage scaled by #{(ce.fraction_mul * 100 / ce.fraction_div * 100)/100}%"
    if ce.fraction_div > ce.fraction_mul
      color = Output::GREEN
    elsif ce.fraction_div < ce.fraction_mul
      color = Output::RED
    else
      color = Output::DEFAULT
    end

    if ce.mat_index >=0
        mat = df.decode_mat(ce.mat_type, ce.mat_index )
    elsif ce.mat_type >= 0
        mat = df.decode_mat(ce.mat_type, 0 )
    else
        mat = nil
    end

    if mat!= nil
      token = mat.token
      if token.start_with?("INORGANIC:")
        token = token.slice("INORGANIC:".length, token.length - "INORGANIC:".length)
      end

      desc = "#{desc} vs #{token.capitalize()}"
    end

  when "BODY_MAT_INTERACTION"
    # interactionId, SundromeTriggerType
    name = "Body material interaction"
    desc = "a???=#{ce.unk_6c}, b???=#{ce.unk_88}, c???=#{ce.unk_8c}, d???=#{ce.unk_90}, e???=#{ce.unk_94}"
    color = Output::DEFAULT
  when "BODY_APPEARANCE_MODIFIER"
    if !showdisplayeffects then return "", Output::DEFAULT end
    # !!! seems to be missing info class !!!
    # should be enum and value
    name = "Body Appearance"
    desc = "<TODO>"
    color = Output::DEFAULT
  when "BP_APPEARANCE_MODIFIER"
    if !showdisplayeffects then return "", Output::DEFAULT end
    name = "Body part appearance"
    desc = "value=#{ce.value} change_type_enum?=#{ce.unk_6c}#{get_effect_target(ce.target)}"
    color = Output::DEFAULT
  when "DISPLAY_NAME"
    if !showdisplayeffects then return "", Output::DEFAULT end
    name = "Set display name"
    desc = "#{ce.name}"
    color = Output::DEFAULT
  else
    name = "Unknown"
    color = Output::HIGHLIGHT
  end

  text = "#{name}#{duration}#{flags} #{desc}"
  if ticks > 0 and ((ce.start > 0     and ticks < ce.start)     or (ce.end > 0     and ticks > ce.end))
    text = logger.inactive(text)
  end

  return text, color
end

print_syndrome = lambda { |logger, syndrome, showeffects, showdisplayeffects|
  rawsyndrome = df.world.raws.syndromes.all[syndrome.type]
  duration = rawsyndrome.ce.minmax_by{ |ce| ce.end }

  if duration[0].end == -1
    durationStr = "permanent"
  else
    if duration[0].end == duration[1].end
      durationStr = "#{syndrome.ticks} of #{duration[0].end}"
    else
      durationStr = "#{syndrome.ticks} of #{duration[0].end}-#{duration[1].end}"
    end
  end

  effects = rawsyndrome.ce.collect { |effect| get_effect(logger, effect, syndrome.ticks, showdisplayeffects) }

  if effects.any?{ |text, color| color==Output::RED }
    color = Output::RED
  elsif effects.any?{|text, color| color==Output::GREEN }
    color = Output::GREEN
  else
    color = Output::DEFAULT
  end

  logger.indent()
  logger.log "#{get_display_name(rawsyndrome.syn_name, "")} [#{durationStr}]", color

  if showeffects
    logger.indent()
    effects.each{ |text, color| if text!="" then logger.log text, color end }
    logger.unindent()
  end
  logger.unindent()
}

print_raw_syndrome = lambda { |logger, rawsyndrome, showeffects, showdisplayeffects|

  effects = rawsyndrome.ce.collect { |effect| get_effect(logger, effect, -1, showdisplayeffects) }

  if effects.any?{ |item| item[1]==Output::RED }
    color = Output::RED
  elsif effects.any?{|item| item[1]==Output::GREEN }
    color = Output::GREEN
  else
    color = Output::DEFAULT
  end

  logger.indent()
  logger.log get_display_name(rawsyndrome.syn_name, ""), color

  if showeffects
    logger.indent()
    effects.each{ |text, color| if text!="" then logger.log text, color end }
    logger.unindent()
  end
  logger.unindent()
}

print_syndromes = lambda { |logger, unit, showrace, showall, showeffects, showhiddencurse, showdisplayeffects|

  if showhiddencurse
    syndromes = unit.syndromes.active
  else
    syndromes = unit.syndromes.active
    # TODO: syndromes = unit.syndromes.active.select{ |s| visible_syndrome?(unit, s) }
  end

  if !syndromes.empty? or showall
    if showrace
      logger.log "#{df.world.raws.creatures.all[unit.race].name[0]}#{unit.name == '' ? "" : ": "}#{unit.name}", Output::HIGHLIGHT
    else
      logger.log "#{unit.name}", Output::HIGHLIGHT
    end
  end

  syndromes.each { |syndrome| print_syndrome[logger, syndrome, showeffects, showdisplayeffects] }
}

def starts_with?(str, prefix)
  prefix = prefix.to_s
  str[0, prefix.length] == prefix
end

showall = false
showeffects = false
selected = false
dwarves = false
livestock = false
wildanimals = false
hostile = false
world = false
showhiddencurse = false
showdisplayeffects = false

if $script_args.any?{ |arg| arg == "help" or arg == "?" or arg == "-?" }
  print_help()
elsif $script_args.empty?
  dwarves = true
  showeffects = true
else
  if $script_args.any?{ |arg| arg == "showall" } then showall=true end
  if $script_args.any?{ |arg| arg == "showeffects" } then showeffects=true end
  if $script_args.any?{ |arg| arg == "ignorehiddencurse" } then showhiddencurse=true end
  if $script_args.any?{ |arg| arg == "showdisplayeffects" } then showdisplayeffects=true end
  if $script_args.any?{ |arg| arg == "selected" } then selected=true end
  if $script_args.any?{ |arg| arg == "dwarves" } then dwarves=true end
  if $script_args.any?{ |arg| arg == "livestock" } then livestock=true end
  if $script_args.any?{ |arg| arg == "wildanimals" } then wildanimals=true end
  if $script_args.any?{ |arg| arg == "hostile" } then hostile=true end
  if $script_args.any?{ |arg| arg == "world" } then world=true end
  if $script_args.any?{ |arg| starts_with?(arg, "export:") }
    exportfile = $script_args.find{ |arg| starts_with?(arg, "export:") }.gsub("export:", "")
    export=true
  end
end

if export
  logger = Output.new(exportfile)
else
  logger = Output.new(nil)
end

if selected
  print_syndromes[logger, df.unit_find(), true, showall, showeffects, showhiddencurse, showdisplayeffects]
  logger.break()
end

if dwarves
  logger.log "Dwarves", Output::HIGHLIGHT
  df.unit_citizens.each { |unit|
    print_syndromes[logger, unit, false, showall, showeffects, showhiddencurse, showdisplayeffects]
  }
  logger.break()
end

if livestock
  logger.log "Livestock", Output::HIGHLIGHT
  df.world.units.active.find_all { |u| df.unit_category(u) == :Livestock }.each { |unit|
    print_syndromes[logger, unit, true, showall, showeffects, showhiddencurse, showdisplayeffects]
  }
  logger.break()
end

if wildanimals
  logger.log "Wild animals", Output::HIGHLIGHT
  df.world.units.active.find_all { |u| df.unit_category(u) == :Other and  df.unit_other_category(u) == :Wild }.each { |unit|
    print_syndromes[logger, unit, true, showall, showeffects, showhiddencurse, showdisplayeffects]
  }
  logger.break()
end

if hostile
  logger.log "Hostile units", Output::HIGHLIGHT
  df.unit_hostiles.each { |unit|
    print_syndromes[logger, unit, true, showall, showeffects, showhiddencurse, showdisplayeffects]
  }
  logger.break()
end

if world
  logger.log "All syndromes", Output::HIGHLIGHT
  df.world.raws.syndromes.all.each { |syndrome| print_raw_syndrome[logger, syndrome, showeffects, showdisplayeffects]    }
end

logger.close()
