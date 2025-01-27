#!/bin/sh

DF_FOLDER=$1
OS_TARGET=$2
DF_VERSION=$3

set -e

minor=$(echo "$DF_VERSION" | cut -d. -f1)
patch=$(echo "$DF_VERSION" | cut -d. -f2)
if [ "$DF_VERSION" = "51.03" ]; then
    patch=02
fi
df_url="https://www.bay12games.com/dwarves/df_${minor}_${patch}"
if test "$OS_TARGET" = "windows"; then
    WGET="C:/msys64/usr/bin/wget.exe"
    df_url="${df_url}_win_s.zip"
    df_archive_name="df.zip"
    df_extract_cmd="unzip -d ${DF_FOLDER}"
elif test "$OS_TARGET" = "ubuntu"; then
    WGET=wget
    df_url="${df_url}_linux.tar.bz2"
    df_archive_name="df.tar.bz2"
    df_extract_cmd="tar -x -j -C ${DF_FOLDER} -f"
else
    echo "Unhandled OS target: ${OS_TARGET}"
    exit 1
fi

if ! $WGET -v "$df_url" -O "$df_archive_name"; then
    echo "Failed to download DF from $df_url"
    exit 1
fi

md5sum "$df_archive_name"

save_url="https://dffd.bay12games.com/download.php?id=15434&f=dreamfort.7z"
save_archive_name="test_save.7z"
save_extract_cmd="7z x -o${DF_FOLDER}/save"

if ! $WGET -v "$save_url" -O "$save_archive_name"; then
    echo "Failed to download test save from $save_url"
    exit 1
fi

md5sum "$save_archive_name"

echo Extracting
mkdir -p ${DF_FOLDER}
$df_extract_cmd "$df_archive_name"
$save_extract_cmd "$save_archive_name"
mv ${DF_FOLDER}/save/* ${DF_FOLDER}/save/region1

echo Done

ls -l
