# -*- coding: utf-8 -*-
try:
    from setuptools import setup, find_packages
except ImportError:
    from ez_setup import use_setuptools
    use_setuptools()
    from setuptools import setup, find_packages
from distutils.core import Extension
from os import path
import platform

if platform.system() == 'Windows':
    # dfhack.lib location can differ, search for it
    for libdir in ["..", path.join("..",".."), path.join("..", "..", "output"), path.join("..", "..", "output", "Release")]:
        if path.isfile(path.join(libdir, "dfhack.lib")):
            lib_dirs = libdir
            break
    else:
        raise Exception("dfhack.lib is not found")
    osspec = dict(library_dirs=lib_dirs)
                    
elif platform.system() == 'Linux':
    osspec = dict(extra_compile_args=["-DLINUX_BUILD", "-w"],
             library_dirs=[path.join("..","..","output")])

e = Extension("pydfhack._pydfhack", 
              sources=["DF_API.cpp", "DF_Buildings.cpp", "DF_Constructions.cpp", "DF_CreatureManager.cpp",\
                       "DF_GUI.cpp", "DF_Maps.cpp", "DF_Material.cpp", "DF_Position.cpp", "DF_Translate.cpp",\
                       "DF_Vegetation.cpp", "pydfhack.cpp"],
              include_dirs=["../", path.join("..", "include"), path.join("..","depends","md5"), path.join("..","depends","tinyxml")],
              libraries=["dfhack"],
              export_symbols=["init_pydfhack", "ReadRaw", "WriteRaw"],
              **osspec)

for file in ["Memory.xml", path.join("..","..","output","Memory.xml")]:
    if path.isfile(file):
        datafile = file
        break
else:
    raise Exception("Memory.xml is not found.")


setup(
    name="PyDFHack",
    description="Python wrapper and bindings for DFHack library",
    version="1.0",
    packages=find_packages(exclude=['ez_setup']),
    data_files=[('pydfhack', [datafile])],
    include_package_data=True,
    test_suite='nose.collector',
    zip_safe=False,
    ext_modules=[e],
    )
