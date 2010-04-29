# -*- coding: utf-8 -*-
from distutils.core import setup, Extension
from os import path

e = Extension("pydfhack", 
	sources=["DF_API.cpp", "DF_Buildings.cpp", "DF_Constructions.cpp", "DF_CreatureManager.cpp", "DF_GUI.cpp", "DF_Maps.cpp", "DF_Material.cpp", "DF_Position.cpp", "DF_Translate.cpp", "DF_Vegetation.cpp", "pydfhack.cpp"],
    include_dirs=["../", path.join("..", "include"), path.join("..","depends","md5"), path.join("..","depends","tinyxml")],
	library_dirs=[path.join("..","..","output")],
    extra_compile_args=["-DLINUX_BUILD", "-w"],
	libraries=["dfhack"],
    export_symbols=["initpydfhack", "ReadRaw", "WriteRaw"])

setup(name="PyDFHack", version="1.0", ext_modules=[e])
