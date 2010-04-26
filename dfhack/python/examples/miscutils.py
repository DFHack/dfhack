_splatter_dict = { 0 : "Rock",
                   1 : "Amber",
                   2 : "Coral",
                   3 : "Green Glass",
                   4 : "Clear Glass",
                   5 : "Crystal Glass",
                   6 : "Ice",
                   7 : "Coal",
                   8 : "Potash",
                   9 : "Ash",
                   10 : "Pearlash",
                   11 : "Lye",
                   12 : "Mud",
                   13 : "Vomit",
                   14 : "Salt",
                   15 : "Filth",
                   16 : "Frozen? Filth",
                   18 : "Grime",
                   0xF2 : "Very Specific Blood (references a named creature)" }

def get_splatter_type(mat1, mat2, creature_types):
    from cStringIO import StringIO
    
    if mat1 in _splatter_dict:
        return _splatter_dict[mat1]
    elif mat1 == 0x2A or mat1 == 0x2B:
        splatter = StringIO()
        
        if mat2 != -1:
            splatter.write(creature_types[mat2]["id"] + " ")

        splatter.write("Blood")

        return splatter.getvalue()
    else:
        return "Unknown"
