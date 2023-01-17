#!/bin/sh

set -e

df_tardest="df.tar.bz2"
save_tardest="test_save.tgz"

cd "$(dirname "$0")"
echo "DF_VERSION: $DF_VERSION"
echo "DF_FOLDER: $DF_FOLDER"
mkdir -p "$DF_FOLDER"
# back out of df_linux
cd "$DF_FOLDER/.."

if ! test -f "$df_tardest"; then
    minor=$(echo "$DF_VERSION" | cut -d. -f2)
    patch=$(echo "$DF_VERSION" | cut -d. -f3)
    echo "Downloading DF $DF_VERSION"
    while read url; do
        echo "Attempting download: ${url}"
        if wget -v "$url" -O "$df_tardest"; then
            break
        fi
    done <<URLS
    https://www.bay12games.com/dwarves/df_${minor}_${patch}_linux.tar.bz2
    https://files.dfhack.org/DF/${minor}.${patch}/df_${minor}_${patch}_linux.tar.bz2
URLS
    echo $df_tardest
    if ! test -f "$df_tardest"; then
        echo "DF failed to download: $df_tardest not found"
        exit 1
    fi

    echo "Downloading test save"
    #test_save_url="https://files.dfhack.org/DF/0.${minor}.${patch}/test_save.tgz"
    test_save_url="https://drive.google.com/uc?export=download&id=1XvYngl-DFONiZ9SD9OC4B2Ooecu8rPFz"
    if ! wget -v "$test_save_url" -O "$save_tardest"; then
        echo "failed to download test save"
        exit 1
    fi
    echo $save_tardest
fi

rm -rf df_linux
mkdir -p df_linux/save

echo Extracting
tar xf "$df_tardest" --strip-components=1 -C df_linux
tar xf "$save_tardest" -C df_linux/save
echo Done

ls -l
