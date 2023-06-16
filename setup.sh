#!/bin/sh

# Stable
CEF_FILE="https://cef-builds.spotifycdn.com/cef_binary_114.2.11%2Bg87c8807%2Bchromium-114.0.5735.134_linux64_minimal.tar.bz2"

#
# download cef
#
if [ ! -d subprojects/cef ]; then
    curl -L ${CEF_FILE} -o subprojects/cef_minimal.tar.bz2
    tar -C subprojects -xf subprojects/cef_minimal.tar.bz2
    rm subprojects/cef_minimal.tar.bz2
    mv subprojects/cef_binary* subprojects/cef
fi

#
# Prepare cef release
#
if [ ! -d $1/Release ]; then
    mkdir $1/Release
    cp -a subprojects/cef/Resources/* $1/Release
    cp -a subprojects/cef/Release/* $1/Release
fi
