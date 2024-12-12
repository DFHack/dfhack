#!/bin/bash

set -e
# Number of jobs == core count
jobs=$(grep -c ^processor /proc/cpuinfo)

# Calculate absolute paths for docker to do mounts
srcdir=$(realpath "$(dirname "$(readlink -f "$0")")"/..)

cd "$srcdir"/build

builder_uid=$(id -u)

mkdir -p win64-cross
mkdir -p win64-cross/output
mkdir -p win64-cross/ccache

# Check for sudo; we want to use the real user
if [[ $(id -u) -eq 0 ]]; then
    if [[ -z "$SUDO_UID" || "$SUDO_UID" -eq 0 ]]; then
        echo "Please don't run this script directly as root, use sudo instead:"
        echo
        echo "  sudo $0"
        # This is because we can't change the buildmaster UID in the container to 0 --
        # that's already taken by root.
        exit 1
    fi

    # If this was run using sudo, let's make sure the directories are owned by the
    # real user (and set the BUILDER_UID to it)
    builder_uid=$SUDO_UID
    chown -R $builder_uid win64-cross
fi

# Pulls the MSVC build env container from the GitHub registry
#
# NOTE: win64-cross is mounted in /src/build due to the hardcoded `cmake ..` in
# the Dockerfile
if ! docker run --rm -i -v "$srcdir":/src -v "$srcdir/build/win64-cross/":/src/build  \
    -e BUILDER_UID=$builder_uid \
    -e CCACHE_DIR=/src/build/ccache \
    -e steam_username \
    -e steam_password \
    --name dfhack-win \
    ghcr.io/dfhack/build-env:master \
    bash -c "cd /src/build && dfhack-configure windows 64 Release -DCMAKE_INSTALL_PREFIX=/src/build/output -DBUILD_DOCS=1 $CMAKE_EXTRA_ARGS && dfhack-make -j$jobs install" \
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
    echo "  cp -r win64-cross/output/* ~/.local/share/Steam/steamapps/common/\"Dwarf Fortress\""
fi
