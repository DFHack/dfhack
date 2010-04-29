# -*- coding: utf-8 -*-
from distutils.core import setup, Extension

e = Extension("pydfhack", 
		sources=["DF_API.cpp", "DF_Buildings.cpp", "DF_Constructions.cpp", "DF_CreatureManager.cpp", "DF_GUI.cpp", "DF_Maps.cpp", "DF_Material.cpp", "DF_Position.cpp", "DF_Translate.cpp", "DF_Vegetation.cpp", "pydfhack.cpp"],
        include_dirs=["../", "../include", "../depends/md5", "../depends/tinyxml"],
		library_dirs=["..\\..\\output"],
        #extra_compile_args=["-w"],
		libraries=["libdfhack"],
        export_symbols=["initpydfhack", "ReadRaw", "WriteRaw"])

setup(name="PyDFHack", version="1.0", ext_modules=[e])
