#!/bin/sh

# usage:
#   ./build-pdf.sh
#   SPHINX=/path/to/sphinx-build ./build-pdf.sh
#   JOBS=3 ./build-pdf.sh ...
# all command-line arguments are passed directly to sphinx-build - run
# ``sphinx-build --help`` for a list, or see
# https://www.sphinx-doc.org/en/master/man/sphinx-build.html

cd $(dirname "$0")
python3 build.py pdf "$@"
