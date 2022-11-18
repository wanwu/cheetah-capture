#!/bin/bash

echo "===== start build ffmpeg-emcc ====="

NOW_PATH=$(cd $(dirname $0); pwd)

WEB_CAPTURE_PATH=$(cd $NOW_PATH/../; pwd)

FFMPEG_PATH=$(cd $WEB_CAPTURE_PATH/script/ffmpeg-3.4.8; pwd)
echo "===============";
echo $FFMPEG_PATH;
echo "=================="
# source $WEB_CAPTURE_PATH/../emsdk/emsdk_env.sh

# rm -rf  $WEB_CAPTURE_PATH/lib/ffmpeg-emcc

mkdir -p $WEB_CAPTURE_PATH/lib/ffmpeg-emcc

cd $FFMPEG_PATH
./configure --disable-x86asm
make clean

apt install clang

CONFIG_ARGS=(
  --prefix=$WEB_CAPTURE_PATH/lib/ffmpeg-emcc \
  --target-os=none        # use none to prevent any os specific configurations
  --arch=x86_32           # use x86_32 to achieve minimal architectural optimization
  --enable-cross-compile  # enable cross compile
  --disable-x86asm        # disable x86 asm
  --disable-asm        # disable x86 asm
  --disable-inline-asm    # disable inline asm
  --disable-stripping     # disable stripping
  --disable-programs      # disable programs build (incl. ffplay, ffprobe & ffmpeg)
  --disable-doc           # disable doc
  --extra-cflags="$CFLAGS"
  --extra-cxxflags="$CFLAGS"
  --extra-ldflags="$LDFLAGS"
  --nm="llvm-nm"
  --ar=emar
  --ranlib=emranlib
  --cc=emcc
  --cxx=em++
  --objcc=emcc
  --dep-cc=emcc
)
cd $FFMPEG_PATH
emconfigure ./configure "${CONFIG_ARGS[@]}"
make -j8

make install

echo "===== finish build ffmpeg-emcc ====="