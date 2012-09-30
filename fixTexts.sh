#!/bin/bash
# this script is used for easy testing of the rst documentation files.
rst2html  Readme.rst Readme.html
rst2html  Compile.rst Compile.html
rst2html  LUA\ Api.rst LUA\ Api.html
rst2html  Contributors.rst > Contributors.html
