#!/bin/bash
# regenerate documentation after editing the .rst files. Requires python and docutils.

force=
while [ $# -gt 0 ]
do
    case "$1" in
        --force) force=1
            ;;
    esac
    shift
done

rst2html=$(which rst2html || which rst2html.py)
if [[ -z "$rst2html" ]]; then
    echo "Docutils not found: See http://docutils.sourceforge.net/"
    exit 1
fi
rst2html_version=$("$rst2html" --version | cut -d' ' -f3)

if [[ $(echo $rst2html_version | cut -d. -f2) -lt 12 ]]; then
    echo "You are using docutils $rst2html_version. Docutils 0.12+ is recommended
to build these documents."
    read -r -n1 -p "Continue? [y/N] " reply
    echo
    if [[ ! $reply =~ ^[Yy]$ ]]; then
        exit
    fi
fi

cd `dirname $0`
status=0

function process() {
    if [ "$1" -nt "$2" ] || [ -n "$force" ]; then
        echo -n "Updating $2... "
        if "$rst2html" --no-generator --no-datestamp "$1" "$2"; then
            echo "Done"
        else
            status=1
        fi
    else
        echo "$2 - up to date."
    fi
}

process Readme.rst Readme.html
process Compile.rst Compile.html
process Lua\ API.rst Lua\ API.html
process Contributors.rst Contributors.html
process Contributing.rst Contributing.html
exit $status
