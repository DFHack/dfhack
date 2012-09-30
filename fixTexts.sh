#!/bin/bash
# regenerate documentation after editing the .rst files. Requires python and docutils.

cd `dirname $0`

function process() {
    if [ "$1" -nt "$2" ]; then
        rst2html --no-generator --no-datestamp "$1" "$2"
    else
        echo "$2 - up to date."
    fi
}

process Readme.rst Readme.html
process Compile.rst Compile.html
process Lua\ API.rst Lua\ API.html
process Contributors.rst Contributors.html

