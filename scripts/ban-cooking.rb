# convenient way to ban cooking categories of food
=begin

ban-cooking
===========
A more convenient way to ban cooking various categories of foods than the
kitchen interface.  Usage:  ``ban-cooking <type>``.  Valid types are ``booze``,
``honey``, ``tallow``, ``oil``, and ``seeds`` (non-tree plants with seeds).

=end

already_banned = {}
kitchen = df.ui.kitchen
kitchen.item_types.length.times { |i|
    already_banned[[kitchen.mat_types[i], kitchen.mat_indices[i], kitchen.item_types[i], kitchen.item_subtypes[i]]] = kitchen.exc_types[i] & 1
}
ban_cooking = lambda { |mat_type, mat_index, type|
    subtype = -1
    key = [mat_type, mat_index, type, subtype]
    if already_banned[key]
        next if already_banned[key] == 1

        index = kitchen.mat_types.zip(kitchen.mat_indices, kitchen.item_types, kitchen.item_subtypes)
        kitchen.exc_types[index] |= 1
        already_banned[key] = 1
        next
    end
    df.ui.kitchen.mat_types     << mat_type
    df.ui.kitchen.mat_indices   << mat_index
    df.ui.kitchen.item_types    << type
    df.ui.kitchen.item_subtypes << subtype
    df.ui.kitchen.exc_types     << 1
    already_banned[key] = 1
}

$script_args.each do |arg|
    case arg
    when 'booze'
        df.world.raws.plants.all.each_with_index do |p, i|
            p.material.each_with_index do |m, j|
                if m.flags[:ALCOHOL]
                    ban_cooking[j + DFHack::MaterialInfo::PLANT_BASE, i, :DRINK]
                end
            end
        end
        df.world.raws.creatures.all.each_with_index do |c, i|
            c.material.each_with_index do |m, j|
                if m.flags[:ALCOHOL]
                    ban_cooking[j + DFHack::MaterialInfo::CREATURE_BASE, i, :DRINK]
                end
            end
        end

    when 'honey'
        # hard-coded in the raws of the mead reaction
        honey = df.decode_mat('CREATURE:HONEY_BEE:HONEY')
        ban_cooking[honey.mat_type, honey.mat_index, :LIQUID_MISC]

    when 'tallow'
        df.world.raws.creatures.all.each_with_index do |c, i|
            c.material.each_with_index do |m, j|
                if m.reaction_product and m.reaction_product.id and m.reaction_product.id.include?('SOAP_MAT')
                    ban_cooking[j + DFHack::MaterialInfo::CREATURE_BASE, i, :GLOB]
                end
            end
        end

    when 'oil'
        df.world.raws.plants.all.each_with_index do |p, i|
            p.material.each_with_index do |m, j|
                if m.reaction_product and m.reaction_product.id and m.reaction_product.id.include?('SOAP_MAT')
                    ban_cooking[j + DFHack::MaterialInfo::PLANT_BASE, i, :LIQUID_MISC]
                end
            end
        end

    when 'seeds'
        df.world.raws.plants.all.each do |p|
            m = df.decode_mat(p.material_defs.type_basic_mat, p.material_defs.idx_basic_mat).material
            ban_cooking[p.material_defs.type_basic_mat, p.material_defs.idx_basic_mat, :PLANT] if m.reaction_product and m.reaction_product.id and m.reaction_product.id.include?('SEED_MAT')

            if not p.flags[:TREE]
                p.growths.each do |g|
                    m = df.decode_mat(g).material
                    ban_cooking[g.mat_type, g.mat_index, :PLANT_GROWTH] if m.reaction_product and m.reaction_product.id and m.reaction_product.id.include?('SEED_MAT')
                end
            end
        end

    else
        puts "ban-cooking booze  - bans cooking of drinks"
        puts "ban-cooking honey  - bans cooking of honey bee honey"
        puts "ban-cooking tallow - bans cooking of tallow"
        puts "ban-cooking oil    - bans cooking of oil"
        puts "ban-cooking seeds  - bans cooking of plants that have seeds (tree seeds don't count)"
    end
end
