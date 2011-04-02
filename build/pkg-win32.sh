#/bin/sh
# Remote into a virtualbox VM to build with MSVC.
# Very specific to my own local setup. ~px

# VARS. TODO: parametrize
export DFHACK_VER=0.5.7
export PKG=dfhack-bin-$DFHACK_VER
export TARGET=Release

# let's build it all
VBoxManage startvm "7 Prof"
sleep 20
expect linux-remote.expect $TARGET

