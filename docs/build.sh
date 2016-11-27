#!/bin/sh

# usage:
# ./build.sh
# ./build.sh sphinx-executable
# JOBS=3 ./build.sh ...

cd $(dirname "$0")
cd ..

sphinx=sphinx-build
if [ -n "$SPHINX" ]; then
    sphinx=$SPHINX
fi

if [ -z "$JOBS" ]; then
    JOBS=2
fi

"$sphinx" -a -E -q -b html . ./docs/html -w ./docs/_sphinx-warnings.txt -j "$JOBS" "$@"
