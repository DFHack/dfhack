#!/bin/sh
cd "$(dirname "$0")"
cd ..
grep DF_VERSION CMakeLists.txt | perl -ne 'print "$&\n" if /[\d\.]+/'
