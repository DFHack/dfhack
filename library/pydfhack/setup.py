from distutils.core import setup, Extension

e = Extension("pydfhack", 
		sources=["UnionBase.cpp", "pydfhack.cpp", "DF_API.cpp"],
        include_dirs=["..\\"],
		library_dirs=["..\\..\\output"],
		libraries=["libdfhack"])

setup(name="PyDFHack", version="1.0", ext_modules=[e])
