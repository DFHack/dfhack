# -*- coding: utf-8 -*-
try:
    from setuptools import setup, find_packages
except ImportError:
    from ez_setup import use_setuptools
    use_setuptools()
    from setuptools import setup, find_packages
from distutils.core import Extension

e = Extension("_pydfhack", 
		sources=["DF_API.cpp", "DF_Buildings.cpp", "DF_Constructions.cpp", "DF_CreatureManager.cpp", "DF_GUI.cpp", "DF_Maps.cpp", "DF_Material.cpp", "DF_Position.cpp", "DF_Translate.cpp", "DF_Vegetation.cpp", "pydfhack.cpp"],
        include_dirs=["../", "../include", "../depends/md5", "../depends/tinyxml"],
		library_dirs=["..\\..\\output"],
        #extra_compile_args=["-w"],
		libraries=["libdfhack"],
        export_symbols=["init_pydfhack", "ReadRaw", "WriteRaw"])

setup(
    name="PyDFHack",
    description="Python wrapper and bindings for DFHack library",
    version="1.0",
    packages=find_packages(exclude=['ez_setup']),
    include_package_data=True,
    test_suite='nose.collector',
    zip_safe=False,
    ext_modules=[e],
    )
