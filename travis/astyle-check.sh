#!/bin/sh

sources=$(find plugins library -type f -regextype posix-awk -regex ".*\.(h|cpp)")

output=$(astyle --options=.astylerc --dry-run -Q $sources)

if [ -n "$output" ]
then
    printf "Files modified by astyle\n$output"
    exit 1
fi
exit 0
