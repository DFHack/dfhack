#!/bin/bash
# regenerate documentation after editing the .rst files. Requires python and docutils.
rst2html --no-generator --no-datestamp Readme.rst Readme.html
rst2html --no-generator --no-datestamp Compile.rst Compile.html
rst2html --no-generator --no-datestamp Lua\ API.rst Lua\ API.html
rst2html --no-generator --no-datestamp Contributors.rst Contributors.html
