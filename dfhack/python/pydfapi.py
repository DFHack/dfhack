import pydfhack, os

class API(pydfhack._API):
	for file in ["Memory.xml", os.path.join("..","..","output","Memory.xml")]:
		if os.path.isfile(file):
			datafile = file
			break
	else:
		raise ImportError, "Memory.xml not found."
	
	def __init__(self, *args, **kwds):
		pydfhack._API.__init__(self, API.datafile)
		
		self._map_mgr_type = Map
		self._vegetation_mgr_type = Vegetation
		self._gui_mgr_type = GUI

class Map(pydfhack._MapManager):
	def __init__(self, *args, **kwds):
		pydfhack._MapManager.__init__(self, args, kwds)

class Vegetation(pydfhack._VegetationManager):
	def __init__(self, *args, **kwds):
		pydfhack._VegetationManager.__init__(self, args, kwds)

class GUI(pydfhack._GUIManager):
	def __init__(self, *args, **kwds):
		pydfhack._GUIManager.__init__(self, args, kwds)
