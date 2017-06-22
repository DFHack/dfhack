#!/bin/sh

set -e

LUA_URL="http://www.lua.org/ftp/lua-5.3.3.tar.gz"
LUA_TAR=$(basename "$LUA_URL")
LUA_DIR="${LUA_TAR%.tar.*}"
LUA_SHA1="a0341bc3d1415b814cc738b2ec01ae56045d64ef"

echo $LUA_TAR
echo $LUA_DIR

sha1() {
    python -c 'import hashlib, sys; print(hashlib.sha1(open(sys.argv[1],"rb").read()).hexdigest())' "$1"
}

download() {
    echo "Downloading $LUA_URL"
    wget -O "$LUA_TAR" "$LUA_URL"
    tar xvf "$LUA_TAR"
}

build() {
    cd "$LUA_DIR/src"
    make generic
}

main() {
    mkdir -p "$HOME/lua53"
    cd "$HOME/lua53"
    mkdir -p bin

    if [ "$(sha1 "$LUA_TAR" 2>/dev/null)" != "$LUA_SHA1" ]; then
        download
        build
    else
        echo "Already downloaded"
    fi

    if [ -x "$LUA_DIR/src/luac" ]; then
        echo "Already built"
    else
        build
    fi

    ln -sf "$(pwd)/$LUA_DIR/src/lua" bin/lua5.3
    ln -sf "$(pwd)/$LUA_DIR/src/luac" bin/luac5.3
}

main
