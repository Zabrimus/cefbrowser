#!/bin/bash


CEF_FILE_X86="https://cef-builds.spotifycdn.com/cef_binary_126.2.7%2Bg300bb05%2Bchromium-126.0.6478.115_linux64_minimal.tar.bz2"
CEF_FILE_ARM64="https://cef-builds.spotifycdn.com/cef_binary_126.2.7%2Bg300bb05%2Bchromium-126.0.6478.115_linuxarm64_minimal.tar.bz2"
CEF_FILE_ARM="https://cef-builds.spotifycdn.com/cef_binary_126.2.7%2Bg300bb05%2Bchromium-126.0.6478.115_linuxarm_minimal.tar.bz2"

# Default
CEF_FILE=${CEF_FILE_X86}

if [ "$1" = "x86" ]; then
  CEF_FILE=${CEF_FILE_X86}
elif [ "$1" = "arm64" ]; then
  CEF_FILE=${CEF_FILE_ARM64}
elif [ "$1" = "arm" ]; then
  CEF_FILE=${CEF_FILE_ARM}
fi

#
# download cef
#
if [ ! -d subprojects/cef ] && [ ! -L subprojects/cef ]; then
    curl -L ${CEF_FILE} -o subprojects/cef_minimal.tar.bz2
    tar -C subprojects -xf subprojects/cef_minimal.tar.bz2
    rm subprojects/cef_minimal.tar.bz2
    mv subprojects/cef_binary* subprojects/cef
fi

#
# Prepare cef release
#
if [[ "$#" -gt 1 ]]; then
  if [ ! -d $2/Release ]; then
      mkdir $2/Release
      cp -a subprojects/cef/Resources/* $2/Release
      cp -a subprojects/cef/Release/* $2/Release
  fi
fi
