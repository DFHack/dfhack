--Toggle idlers count
--[====[

tidlers
=======
Toggle between all possible positions where the idlers count can be placed
(see ``data/init/d_init.txt`` for details).

]====]
df.global.d_init.idlers = df.d_init_idlers:next_item(df.global.d_init.idlers)
print('Toggled the display of idlers.')
