#/bin/sh

# VARS
export WINEARCH=win32
export WINEPREFIX=$HOME/.wine-mscv/
export WINEDEBUG=-all
export DFHACK_VER=0.5.6
export PKG=dfhack-bin-$DFHACK_VER
export VCBUILD="/home/peterix/.wine-mscv/drive_c/Program Files/Microsoft Visual Studio 9.0/VC/vcpackages/vcbuild.exe"
export TARGET=Release
export PROJECTS="library\\dfhack.vcproj
tools\\supported\\dfattachtest.vcproj
tools\\supported\\dfautosearch.vcproj
tools\\supported\\dfcleanmap.vcproj
tools\\supported\\dfcleartask.vcproj
tools\\supported\\dfdoffsets.vcproj
tools\\supported\\dfexpbench.vcproj
tools\\supported\\dfflows.vcproj
tools\\supported\\dfincremental.vcproj
tools\\supported\\dfliquids.vcproj
tools\\supported\\dfmode.vcproj
tools\\supported\\dfpause.vcproj
tools\\supported\\dfposition.vcproj
tools\\supported\\dfprobe.vcproj
tools\\supported\\dfprospector.vcproj
tools\\supported\\dfreveal.vcproj
tools\\supported\\dfsuspend.vcproj
tools\\supported\\dfunstuck.vcproj
tools\\supported\\dfvdig.vcproj
tools\\supported\\dfweather.vcproj"

# let's build it all
rm -r build-real
mkdir build-real
cd build-real
wine cmake ..\\.. -G"Visual Studio 9 2008"
for proj in $PROJECTS
do
wine "$VCBUILD" $proj $TARGET
done

echo "Creating package..."
cd ../../output/$TARGET
rm -r $PKG
rm $PKG.zip
mkdir $PKG
mv *.exe *.dll *.html *.txt *.xml $PKG
zip -r $PKG.zip $PKG
echo "DONE"