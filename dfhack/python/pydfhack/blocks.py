class Point(object):
    x = None
    y = None
    z = None
    block = False
    def __init__(self, x, y, z, block=False):
        self.x = x
        self.y = y
        self.z = z
        self.block = block
    
    def get_block(self):
        return Point(self.x/16, self.y/16, self.z, True)
       
class Block(object):
    """
    16x16 tiles block
    """
    api = None
    tiles = None
    coords = None
    def __init__(self, api, coords):
        """
        api is instance of API, which is used for read/write operations
        coords is Point object
        """
        self.api = api
        if not isinstance(Point, coords):
            raise Exception(u"coords parameter should be Point")
            
        if not coords.block:
            coords = coords.get_block()
        self.coords = coords

    def reload(self):
        pass

    def save(self):
        pass
  
