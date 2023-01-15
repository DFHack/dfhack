#!/usr/bin/env sh

# Number of jobs == core count
jobs=$(grep -c ^processor /proc/cpuinfo)

# Calculate absolute paths for docker to do mounts
srcdir=$(realpath "$(dirname "$(readlink -f "$0")")"/..)

cd "$srcdir"/build

mkdir -p win64-cross
mkdir -p win64-cross/output

# Assumes you built a container image called dfhack-build-msvc from
# https://github.com/BenLubar/build-env/tree/master/msvc, see
# docs/dev/compile/Compile.rst.
#
# NOTE: win64-cross is mounted in /src/build due to the hardcoded `cmake ..` in
# the Dockerfile
#
# TODO: make this work for rootless docker, i.e. remove the sudo for those that
# don't normally need to use sudo to run docker.
if ! sudo docker run --rm -it -v "$srcdir":/src -v "$srcdir/build/win64-cross/":/src/build  \
    --user buildmaster \
    --name dfhack-win \
    dfhack-build-msvc bash -c "cd /src/build && dfhack-configure windows 64 Release -DCMAKE_INSTALL_PREFIX=/src/build/output && dfhack-make -j$jobs install" \
    ; then
    echo
    echo "Build failed"
    exit 1
else
    echo
    echo "Windows artifacts are at win64-cross/output. Copy or symlink them to"
    echo "your steam DF directory to install dfhack (and optionally delete the"
    echo "hack/ directory already present)"
    echo
    echo "Typically this can be done like this:"
    echo "  cp -r win64-cross/output/* \"$HOME/.local/share/Steam/steamapps/common/Dwarf Fortress\""
fi
