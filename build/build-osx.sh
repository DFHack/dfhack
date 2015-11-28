#!/bin/bash

# This implements steps 7 and 8 of the OSX compilation procedure described in Compile.rst
# If build-osx does not exist in the parent directory, it will be created.

function lua_patch() { action="$1" LUA_PATCH="$2"
  LUA_PATCH_CODE="
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
         }"

  if [[ $LUA_PATCH -ne 0 ]]
  then
    (
      cd ..
      echo -n "Patch Lua code: "
      if [[ "$action" == "remove" ]]
      then
        echo "removing patch."
        patch -p0 <<< "$LUA_PATCH_CODE" -R
      elif [[ "$action" == "apply" ]]
      then
        echo "applying patch."
        patch -p0 <<< "$LUA_PATCH_CODE"
      else
        echo "Error in script: unknown lua_patch action"
        exit 1
      fi
    )
  fi
}

DEFAULT_GCC_VER=4.5
GCC_VER="$DEFAULT_GCC_VER"
PARALLEL_PROC=1
LUA_PATCH=1

targetted=0

usage() {
  echo "Usage: $0 [-gcc version] [-par #] [old] [brew|port] [clean] <DF_OSX_PATH>"
  echo -e "\t-gcc <ver>\t-use GCC version <ver> instead of the default $DEFAULT_GCC_VER"
  echo -e "\t-par #\t-build with # parallel processes (default = $PARALLEL_PROC)"
  echo -e "\told\t- use on pre-Snow Leopard OSX installations"
  echo -e "\tbrew\t- if GCC $GCC_VER was installed with homebrew"
  echo -e "\tport\t- if GCC $GCC_VER was insalled with macports"
  echo -e "\tclean\t- delete ../build-osx before compiling"
  echo "Examples:"
  echo -e "\t$0 old brew ../../../personal/df_osx"
  echo -e "\t$0 port clean /Users/dfplayer/df_osx"
  echo -e "\t$0 port -gcc 4.9 -par 4 /Users/dfplayer/df_osx"
  echo -e "\t$0 -gcc 5 brew clean \~/games/df_osx"
  exit $1
}

while (( $# > 1 )); do
  case "$1" in
    -gcc)
      GCC_VER="$2"
      [[ "$GCC_VER" == *[^0-9.]* ]] && { echo "Invalid GCC version specified: $GCC_VER" ; exit 1 ; }
      echo "Changing GCC version from $DEFAULT_GCC_VER to $GCC_VER"
      shift ; shift
      ;;
    -par)
      PARALLEL_PROC="$2"
      [[ "$PARALLEL_PROC" == *[^0-9]* ]] && { echo "Invalid parallel process specified: $PARALLEL_PROC" ; exit 1 ; }
      echo "Changing parallel process count to $PARALLEL_PROC"
      shift ; shift
      ;;
    brew)
      echo "Using homebrew gcc."
      export CC=/usr/local/bin/gcc-$GCC_VER
      export CXX=/usr/local/bin/g++-$GCC_VER
      targetted=1
      shift
      ;;
      port)
      echo "Using macports gcc."
      export CC=/opt/local/bin/gcc-mp-$GCC_VER
      export CXX=/opt/local/bin/g++-mp-$GCC_VER
      targetted=1
      shift
      ;;
      old)
      LUA_PATCH=0
      shift
      ;;
      clean)
      echo "Deleting ../build-osx"
      rm -rf ../build-osx
      shift
      ;;
  esac
done

# sanity checks
if (( $# < 1 ))
then
  echo "Not enough arguments."
  usage 0
fi
if (( $# > 5 ))
then
  echo "Too many arguments."
  usage 1
fi

if (( $targetted == 0 ))
then
  echo "You did not specify whether you intalled GCC $GCC_VER from brew or ports."
  echo "If you continue, your default compiler will be used."
  read -p "Are you sure you want to continue? [y/N] " -n 1 -r
  echo    # (optional) move to a new line
  if [[ ! $REPLY =~ ^[Yy]$ ]]
  then
    exit 0
  fi
fi

echo "Installing to: $1"

if [[ ! -d "$1" ]]
then
  echo "ERROR: Specified installation directory $1 does not exist or is not a directory!"
  exit 1
fi

# Only check that CC/CXX exist if $CC is non-empty, to allow for default compiler usage
# when not specifying brew/port.
if [[ ! -z "$CC" ]]
then
  echo "Compiler executables to be used:"
  echo -e "\tCC: $CC"
  echo -e "\tCXX: $CXX"

  if [[ ! -x "$CC" || ! -x "$CXX" ]]
  then
    echo "ERROR: Cannot find compiler executable(s), or they are not executable!"
    exit 1
  fi
fi

# check for build folder and start working there
if [[ ! -d ../build-osx ]]
then
  mkdir ../build-osx
fi
cd ../build-osx

# patch if necessary
lua_patch apply "$LUA_PATCH"

# Run the build and time it.
export TIMEFORMAT="%lR"
# Bash redirection magic to capture the duration of the commands to a variable, without losing their stdout/err.
{ build_time=$(
  { time {
      echo "Generate"
      cmake .. -DCMAKE_BUILD_TYPE:string=Release -DCMAKE_INSTALL_PREFIX="$1"
      echo "Build"
      make -j"$PARALLEL_PROC"
      echo "Install"
      make install ; } 1>&3- 2>&4- ; } 2>&1 ); } 3>&1 4>&2

echo "Build finished."
echo "Build duration: $build_time"

if [[ "$GCC_VER" != "$DEFAULT_GCC_VER" ]]
then
  echo "Reminder: you compiled with GCC $GCC_VER, which is not the default of GCC $DEFAULT_GCC_VER"
  echo "You will need to copy or symlink your compiler's i386/32bit libstdc++.6.dylib into $1/hack/"
fi

# unpatch if /libarary/luaTypes.cpp was patched
lua_patch remove "$LUA_PATCH"

exit 0
