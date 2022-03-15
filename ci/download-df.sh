#!/bin/sh

set -e

tardest="df.tar.bz2"

selfmd5=$(openssl md5 < "$0")
echo $selfmd5

cd "$(dirname "$0")"
echo "DF_VERSION: $DF_VERSION"
echo "DF_FOLDER: $DF_FOLDER"
mkdir -p "$DF_FOLDER"
# back out of df_linux
cd "$DF_FOLDER/.."

if [ -f receipt ]; then
    if [ "$selfmd5" != "$(cat receipt)" ]; then
        echo "download-df.sh changed; removing DF"
        rm receipt
    else
        echo "Already downloaded $DF_VERSION"
    fi
fi

if [ ! -f receipt ]; then
    rm -f "$tardest"
    minor=$(echo "$DF_VERSION" | cut -d. -f2)
    patch=$(echo "$DF_VERSION" | cut -d. -f3)
    url="http://www.bay12games.com/dwarves/df_${minor}_${patch}_linux.tar.bz2"
    echo Downloading
    while read url; do
        echo "Attempting download: ${url}"
        if wget -v "$url" -O "$tardest"; then
            break
        fi
    done <<URLS
    https://www.bay12games.com/dwarves/df_${minor}_${patch}_linux.tar.bz2
    https://files.dfhack.org/DF/0.${minor}.${patch}/df_${minor}_${patch}_linux.tar.bz2
URLS
    echo $tardest
    if ! test -f "$tardest"; then
        echo "DF failed to download: $tardest not found"
        exit 1
    fi
fi

rm -rf df_linux
mkdir df_linux

echo Extracting
tar xf "$tardest" --strip-components=1 -C df_linux
echo Done

echo "$selfmd5" > receipt
ls
