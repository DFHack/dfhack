#!/bin/sh

# usage:
#   ./build.sh
#   SPHINX=/path/to/sphinx-build ./build.sh
#   JOBS=3 ./build.sh ...
# all command-line arguments are passed directly to sphinx-build - run
# ``sphinx-build --help`` for a list, or see
# https://www.sphinx-doc.org/en/master/man/sphinx-build.html

cd $(dirname "$0")
cd ..

"${SPHINX:-sphinx-build}" -b html -d build/docs/html . docs/html -w build/docs/html/_sphinx-warnings.txt -j "${JOBS:-auto}" "$@"
"${SPHINX:-sphinx-build}" -b text -d build/docs/text . docs/text -w build/docs/text/_sphinx-warnings.txt -j "${JOBS:-auto}" "$@"
