#!/bin/bash

cd `dirname $0`

function process() {
    if [ "$1" -nt "$2" ]; then
        rst2html "$1" "$2"
    else
        echo "$2 - up to date."
    fi
}

# this script is used for easy testing of the rst documentation files.
process Readme.rst Readme.html
process Compile.rst Compile.html
process Lua\ API.rst Lua\ API.html
process Contributors.rst Contributors.html
