# setup stuff to allow arena creature spawn after a mode change

df.world.arena_spawn.race.clear
df.world.arena_spawn.caste.clear

df.world.raws.creatures.all.length.times { |r_idx|
    df.world.raws.creatures.all[r_idx].caste.length.times { |c_idx|
        df.world.arena_spawn.race << r_idx
               df.world.arena_spawn.caste << c_idx
    }
}

df.world.arena_spawn.creature_cnt[df.world.arena_spawn.race.length-1] = 0

puts <<EOS
=begin

devel/spawn-unit-helper
=======================
Setup stuff to allow arena creature spawn after a mode change.

With Arena spawn data initialized:

- enter the :kbd:`k` menu and change mode using
  ``rb_eval df.gametype = :DWARF_ARENA``

- spawn creatures (:kbd:`c` ingame)

- revert to game mode using ``rb_eval df.gametype = #{df.gametype.inspect}``

- To convert spawned creatures to livestock, select each one with
  the :kbd:`v` menu, and enter ``rb_eval df.unit_find.civ_id = df.ui.civ_id``

=end
EOS
