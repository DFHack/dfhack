#/bin/sh

# VARS
export DFHACK_VER=0.5.7
export PKG=dfhack-bin-$DFHACK_VER
export TARGET=Release

# let's build it all
VBoxManage startvm "7 Prof"
sleep 5
expect buildremote.expect $TARGET

echo "Creating package..."
cd ../output/$TARGET
rm -r $PKG
rm $PKG.zip
mkdir $PKG
mv *.exe *.dll *.html *.txt *.xml $PKG
zip -r $PKG.zip $PKG
echo "DONE"
