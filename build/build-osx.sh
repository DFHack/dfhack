#!/bin/bash

# This implements steps 7 and 8 of the OSX compilation procedure described in Compile.rst
# If build-osx does not exist in the parent directory, it will be created.

LUA_PATCH=1
ME=$PWD/`basename $0`

usage() {
	echo "Usage: $0 [options] {DF_OSX_PATH}"
	echo -e "\told\t- use on pre-Snow Leopard OSX installations"
	echo -e "\tbrew\t- if GCC 4.5 was installed with homebrew"
	echo -e "\tport\t- if GCC 4.5 was insalled with macports"
	echo -e "\tclean\t- delete ../build-osx before compiling"
	echo "Example:"
	echo -e "\t$0 old brew ../../../personal/df_osx"
	echo -e "\t$0 port clean /Users/dfplayer/df_osx"
	exit $1
}

options() {
	case $1 in
		brew)
			echo "Using homebrew gcc."
			export CC=/usr/local/bin/gcc-4.5
			export CXX=/usr/local/bin/g++-4.5
			targetted=1
			;;
		port)
			echo "Using macports gcc."
			export CC=/opt/local/bin/gcc-mp-4.5
			export CXX=/opt/local/bin/g++-mp-4.5
			targetted=1
			;;
		old)
			LUA_PATCH=0
			;;
		clean)
			echo "Deleting ../build-osx"
			rm -rf ../build-osx
			;;
		*)
			;;
	esac
}

# sanity checks
if [[ $# -lt 1 ]]
then
	echo "Not enough arguments."
	usage 0
fi
if [[ $# -gt 4 ]]
then
	echo "Too many arguments."
	usage 1
fi

# run through the arguments
for last
do
	options $last
done
# last keeps the last argument

if [[ $targetted -eq 0 ]]
then
	echo "You did not specify whether you intalled GCC 4.5 from brew or ports."
	echo "If you continue, your default compiler will be used."
	read -p "Are you sure you want to continue? [y/N] " -n 1 -r
	echo    # (optional) move to a new line
	if [[ ! $REPLY =~ ^[Yy]$ ]]
	then
	    exit 0
	fi
fi

# check for build folder and start working there
if [[ ! -d ../build-osx ]]
then
	mkdir ../build-osx
fi
cd ../build-osx

# patch if necessary
if [[ $LUA_PATCH -ne 0 ]]
then
	cd ..
	echo "$PWD"
	sed -e '1,/'"PATCH""CODE"'/d' "$ME" | patch -p0
	cd -
fi

echo "Generate"
cmake .. -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX="$last"
echo "Build"
make
echo "Install"
make install

# unpatch if /libarary/luaTypes.cpp was patched
if [[ $LUA_PATCH -ne 0 ]]
then
	cd ..
	echo -n "un"
	sed -e '1,/'"PATCH""CODE"'/d' "$ME" | patch -p0 -R
	cd -
fi

exit 0

# PATCHCODE - everything below this line is fed into patch
--- library/LuaTypes.cpp	2014-08-20 00:13:17.000000000 -0700
+++ library/LuaTypes.cpp	2014-08-31 23:31:00.000000000 -0700
@@ -464,7 +464,7 @@
     {
         case struct_field_info::STATIC_STRING:
         {
-            int len = strnlen((char*)ptr, field->count);
+            int len = strlen((char*)ptr);
             lua_pushlstring(state, (char*)ptr, len);
             return;
         }
