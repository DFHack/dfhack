import sys
from pydfhack import ContextManager

df_cm = ContextManager("Memory.xml")
df = df_cm.get_single_context()

if not df.attach():
    print "Unable to attach!"
    print "Press any key to continue"
    
    raw_input()
    sys.exit(1)

gui = df.gui

if gui is not None:
    maps = df.maps
    world = df.world

    have_maps = maps.start()
    world.start()

    gm = world.read_game_mode()

    if gm is not None:
        print gm

    date_tuple = (world.read_current_year(), world.read_current_month(), world.read_current_day(), world.read_current_tick())

    print "Year: %d Month: %d Day: %d Tick: %d" % date_tuple

    v_coords = gui.get_view_coords()
    c_coords = gui.get_cursor_coords()
    w_coords = (-1, -1, -1)
    world_pos_string = ""

    if have_maps is True:
        w_coords = maps.getPosition()
        
        x = (v_coords[0] + w_coords[0]) * 48
        y = (v_coords[1] + w_coords[1]) * 48
        z = (v_coords[2] + w_coords[2])
        
        world_pos_string = "      world: %d/%d/%d" % (x, y, z)
        
        print "Map world offset: %d/%d/%d embark squares" % w_coords

    if v_coords != (-1, -1, -1):
        print "view coords: %d/%d/%d" % v_coords
        
        if have_maps is True:
            print world_pos_string

    if c_coords != (-1, -1, -1):
        print "cursor coords: %d/%d/%d" % c_coords
        
        if have_maps is True:
            print world_pos_string

    window_size = gui.get_window_size()

    if window_size != (-1, -1):
        print "window size : %d %d" % window_size
else:
    print "cursor and window parameters are unsupported on your version of DF"

if not df.detach():
    print "Unable to detach!"

print "Done.  Press any key to continue"
raw_input()