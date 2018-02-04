#!/bin/sh

tardest="df.tar.bz2"

which md5sum && alias md5=md5sum
selfmd5=$(openssl md5 < "$0")
echo $selfmd5

cd "$(dirname "$0")"
echo "DF_VERSION: $DF_VERSION"
echo "DF_FOLDER: $DF_FOLDER"
mkdir -p "$DF_FOLDER"
cd "$DF_FOLDER"

if [ -f receipt ]; then
    if [ "$selfmd5" != "$(cat receipt)" ]; then
        echo "download-df.sh changed; removing DF"
    else
        echo "Already downloaded $DF_VERSION"
        exit 0
    fi
fi

rm -rif "$tardest" df_linux

minor=$(echo "$DF_VERSION" | cut -d. -f2)
patch=$(echo "$DF_VERSION" | cut -d. -f3)
url="http://www.bay12games.com/dwarves/df_${minor}_${patch}_linux.tar.bz2"

echo Downloading
wget "$url" -O "$tardest"
echo Extracting
tar xf "$tardest" --strip-components=1
echo Changing settings
echo '' >> "$DF_FOLDER/data/init/init.txt"
echo '[PRINT_MODE:TEXT]' >> "$DF_FOLDER/data/init/init.txt"
echo '[SOUND:NO]' >> "$DF_FOLDER/data/init/init.txt"
echo Done

echo "$selfmd5" > receipt
