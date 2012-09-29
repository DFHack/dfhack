#!/bin/bash
# this script is used for easy testing of the rst documentation files.
rst2html  README.rst > README.html
rst2html  COMPILE.rst > COMPILE.html
rst2html  LUA_API.rst > LUA_API.html
