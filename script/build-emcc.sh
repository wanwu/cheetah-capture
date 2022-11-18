#!/bin/bash -x
###
 # @file: 
 # @Author: jiangzichen01@baidu.com
 # @LastEditors: jiangzichen01@baidu.com
 # @Date: 2022-11-07 16:39:36
###

# verify Emscripten version

NOW_PATH=$(cd $(dirname $0); pwd)
# 先
WEB_CAPTURE_PATH=$(cd $NOW_PATH/../; pwd)
FFMPEG_PATH=$(cd $WEB_CAPTURE_PATH/../ffmpeg-5.1; pwd)
echo $FFMPEG_PATH;
source $WEB_CAPTURE_PATH/../emsdk/emsdk_env.sh
emcc -v
emcc --clear-cache
# configure FFMpeg with Emscripten
CFLAGS="-s USE_PTHREADS"
LDFLAGS="$CFLAGS -s INITIAL_MEMORY=33554432" # 33554432 bytes = 32 MB
CONFIG_ARGS=(
  --prefix=$WEB_CAPTURE_PATH/lib/ffmpeg-emcc \
  --target-os=none        # use none to prevent any os specific configurations
  --arch=x86_32           # use x86_32 to achieve minimal architectural optimization
  --enable-cross-compile  # enable cross compile
  --disable-x86asm        # disable x86 asm
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

# # build dependencies
# emmake make -j6

# # build ffmpeg.wasm
# mkdir -p wasm/dist
# ARGS=(
#   -I. -I./fftools
#   -Llibavcodec -Llibavdevice -Llibavfilter -Llibavformat -Llibavresample -Llibavutil -Llibpostproc -Llibswscale -Llibswresample
#   -Qunused-arguments
#   -o wasm/dist/ffmpeg.js fftools/ffmpeg_opt.c fftools/ffmpeg_filter.c fftools/ffmpeg_hw.c fftools/cmdutils.c fftools/ffmpeg.c
#   -lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lavutil -lm
#   -s USE_SDL=2                    # use SDL2
#   -s USE_PTHREADS=1               # enable pthreads support
#   -s INITIAL_MEMORY=33554432      # 33554432 bytes = 32 MB
# )
# emcc "${ARGS[@]}"