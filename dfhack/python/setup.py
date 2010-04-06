# -*- coding: utf-8 -*-
from distutils.core import setup, Extension

e = Extension("pydfhack", 
		sources=["DF_MemInfo.cpp", "DF_API.cpp", "pydfhack.cpp"],
        include_dirs=["..\\", "..\\include", "..\\depends\\md5", "..\\depends\\tinyxml"],
		library_dirs=["..\\..\\output"],
		libraries=["libdfhack"],
        export_symbols=["initpydfhack", "ReadRaw", "WriteRaw"])

setup(name="PyDFHack", version="1.0", ext_modules=[e])
