module DFHack
    class MaterialInfo
        attr_accessor :mat_type, :mat_index
        attr_accessor :mode, :material, :creature, :figure, :plant, :inorganic
        def initialize(what, idx=nil)
            case what
            when Integer
                @mat_type, @mat_index = what, idx
                decode_type_index
            when String
                decode_string(what)
            else
                @mat_type, @mat_index = what.mat_type, what.mat_index
                decode_type_index
            end
        end

        CREATURE_BASE = 19
        FIGURE_BASE = CREATURE_BASE+200
        PLANT_BASE = FIGURE_BASE+200
        END_BASE = PLANT_BASE+200

        # interpret the mat_type and mat_index fields
        def decode_type_index
            if @mat_index < 0 or @mat_type >= END_BASE
                @mode = :Builtin
                @material = df.world.raws.mat_table.builtin[@mat_type]

            elsif @mat_type >= PLANT_BASE
                @mode = :Plant
                @plant = df.world.raws.plants.all[@mat_index]
                @material = @plant.material[@mat_type-PLANT_BASE] if @plant

            elsif @mat_type >= FIGURE_BASE
                @mode = :Figure
                @figure = df.world.history.figures.binsearch(@mat_index)
                @creature = df.world.raws.creatures.all[@figure.race] if @figure
                @material = @creature.material[@mat_type-FIGURE_BASE] if @creature

            elsif @mat_type >= CREATURE_BASE
                @mode = :Creature
                @creature = df.world.raws.creatures.all[@mat_index]
                @material = @creature.material[@mat_type-CREATURE_BASE] if @creature

            elsif @mat_type > 0
                @mode = :Builtin
                @material = df.world.raws.mat_table.builtin[@mat_type]

            elsif @mat_type == 0
                @mode = :Inorganic
                @inorganic = df.world.raws.inorganics[@mat_index]
                @material = @inorganic.material if @inorganic
            end
        end

        def decode_string(str)
            parts = str.split(':')
            case parts[0].chomp('_MAT')
            when 'INORGANIC', 'STONE', 'METAL'
                decode_string_inorganic(parts)
            when 'PLANT'
                decode_string_plant(parts)
            when 'CREATURE'
                if parts[3] and parts[3] != 'NONE'
                    decode_string_figure(parts)
                else
                    decode_string_creature(parts)
                end
            when 'INVALID'
                @mat_type = parts[1].to_i
                @mat_index = parts[2].to_i
            else
                decode_string_builtin(parts)
            end
        end

        def decode_string_inorganic(parts)
            @@inorganics_index ||= (0...df.world.raws.inorganics.length).inject({}) { |h, i| h.update df.world.raws.inorganics[i].id => i }

            @mode = :Inorganic
            @mat_type = 0

            if parts[1] and parts[1] != 'NONE'
                @mat_index = @@inorganics_index[parts[1]]
                raise "invalid inorganic token #{parts.join(':').inspect}" if not @mat_index
                @inorganic = df.world.raws.inorganics[@mat_index]
                @material = @inorganic.material
            end
        end

        def decode_string_builtin(parts)
            @@builtins_index ||= (1...df.world.raws.mat_table.builtin.length).inject({}) { |h, i| b = df.world.raws.mat_table.builtin[i] ; b ? h.update(b.id => i) : h }

            @mode = :Builtin
            @mat_index = -1
            @mat_type = @@builtins_index[parts[0]]
            raise "invalid builtin token #{parts.join(':').inspect}" if not @mat_type
            @material = df.world.raws.mat_table.builtin[@mat_type]

            if parts[0] == 'COAL' and parts[1]
                @mat_index = ['COKE', 'CHARCOAL'].index(parts[1]) || -1
            end
        end

        def decode_string_creature(parts)
            @@creatures_index ||= (0...df.world.raws.creatures.all.length).inject({}) { |h, i| h.update df.world.raws.creatures.all[i].creature_id => i }

            @mode = :Creature

            if parts[1] and parts[1] != 'NONE'
                @mat_index = @@creatures_index[parts[1]]
                raise "invalid creature token #{parts.join(':').inspect}" if not @mat_index
                @creature = df.world.raws.creatures.all[@mat_index]
            end

            if @creature and parts[2] and parts[2] != 'NONE'
                @mat_type = @creature.material.index { |m| m.id == parts[2] }
                @material = @creature.material[@mat_type]
                @mat_type += CREATURE_BASE
            end
        end

        def decode_string_figure(parts)
            @mode = :Figure
            @mat_index = parts[3].to_i
            @figure = df.world.history.figures.binsearch(@mat_index)
            raise "invalid creature histfig #{parts.join(':').inspect}" if not @figure

            @creature = df.world.raws.creatures.all[@figure.race]
            if parts[1] and parts[1] != 'NONE'
                raise "invalid histfig race #{parts.join(':').inspect}" if @creature.creature_id != parts[1]
            end

            if @creature and parts[2] and parts[2] != 'NONE'
                @mat_type = @creature.material.index { |m| m.id == parts[2] }
                @material = @creature.material[@mat_type]
                @mat_type += FIGURE_BASE
            end
        end

        def decode_string_plant(parts)
            @@plants_index ||= (0...df.world.raws.plants.all.length).inject({}) { |h, i| h.update df.world.raws.plants.all[i].id => i }

            @mode = :Plant

            if parts[1] and parts[1] != 'NONE'
                @mat_index = @@plants_index[parts[1]]
                raise "invalid plant token #{parts.join(':').inspect}" if not @mat_index
                @plant = df.world.raws.plants.all[@mat_index]
            end

            if @plant and parts[2] and parts[2] != 'NONE'
                @mat_type = @plant.material.index { |m| m.id == parts[2] }
                raise "invalid plant type #{parts.join(':').inspect}" if not @mat_type
                @material = @plant.material[@mat_type]
                @mat_type += PLANT_BASE
            end
        end

        # delete the caches of raws id => index used in decode_string
        def self.flush_raws_cache
            @@inorganics_index = @@plants_index = @@creatures_index = @@builtins_index = nil
        end

        def token
            out = []
            case @mode
            when :Builtin
                out << (@material ? @material.id : 'NONE')
                out << (['COKE', 'CHARCOAL'][@mat_index] || 'NONE') if @material and @material.id == 'COAL' and @mat_index >= 0
            when :Inorganic
                out << 'INORGANIC'
                out << @inorganic.id if @inorganic
            when :Plant
                out << 'PLANT_MAT'
                out << @plant.id if @plant
                out << @material.id if @plant and @material
            when :Creature, :Figure
                out << 'CREATURE_MAT'
                out << @creature.creature_id if @creature
                out << @material.id if @creature and @material
                out << @figure.id.to_s if @creature and @material and @figure
            else
                out << 'INVALID'
                out << @mat_type.to_s
                out << @mat_index.to_s
            end
            out.join(':')
        end

        def to_s ; token ; end

        def ===(other)
            other.mat_index == mat_index and other.mat_type == mat_type
        end
    end

    class << self
        def decode_mat(what, idx=nil)
            MaterialInfo.new(what, idx)
        end
    end
end
