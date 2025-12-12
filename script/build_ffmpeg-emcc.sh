#!/bin/bash

# before run this script, you need to download ffmpeg-3.4.8.tar.bz2 and extract it to the script directory
# wget https://ffmpeg.org/releases/ffmpeg-3.4.8.tar.bz2
# tar -xjf ffmpeg-3.4.8.tar.bz2
echo "===== start build ffmpeg-emcc ====="

NOW_PATH=$(cd $(dirname $0); pwd)

WEB_CAPTURE_PATH=$(cd $NOW_PATH/../; pwd)

FFMPEG_PATH=$(cd $WEB_CAPTURE_PATH/script/ffmpeg-3.4.8; pwd)
echo "===============";
echo $FFMPEG_PATH;
echo "=================="

# active emsdk
EMSDK_PATH="$WEB_CAPTURE_PATH/emsdk"
if [ -f "$EMSDK_PATH/emsdk_env.sh" ]; then
    echo "===== Activating emsdk from $EMSDK_PATH ====="
    source "$EMSDK_PATH/emsdk_env.sh"
else
    echo "ERROR: emsdk not found at $EMSDK_PATH"
    echo "Please install emsdk first"
    exit 1
fi

# verify emcc is available
if ! command -v emcc &> /dev/null; then
    echo "ERROR: emcc command not found after activating emsdk"
    exit 1
fi

echo "===== emcc version: $(emcc --version | head -1) ====="

mkdir -p $WEB_CAPTURE_PATH/lib/ffmpeg-emcc

cd $FFMPEG_PATH

  # clean previous build artifacts and configurations
make distclean 2>/dev/null || true

CONFIG_ARGS=(
  --prefix=$WEB_CAPTURE_PATH/lib/ffmpeg-emcc \
  --target-os=none        # use none to prevent any os specific configurations
  --arch=x86_32           # use x86_32 to achieve minimal architectural optimization
  --enable-cross-compile  # enable cross compile
  --disable-x86asm        # disable x86 asm
  --disable-asm           # disable asm
  --disable-inline-asm    # disable inline asm
  --disable-stripping     # disable stripping
  --disable-programs      # disable programs build (incl. ffplay, ffprobe & ffmpeg)
  --disable-doc           # disable doc
  --disable-network       # disable network (not needed for local file capture)

  # ===== important: disable all components, then selectively enable =====
  --disable-everything    # disable all components first

  # ===== disable unnecessary libraries (reduce wasm size) =====
  --disable-avdevice      # disable device input/output
  --disable-avfilter      # disable filter
  --disable-postproc      # disable post-processing
  --disable-swresample    # disable audio resampling

  # ===== enable video decoders (only keep the ones needed for frame capture) =====
  --enable-decoder=h264         # H.264/AVC
  --enable-decoder=hevc         # H.265/HEVC
  --enable-decoder=vp8          # VP8
  --enable-decoder=vp9          # VP9
  --enable-decoder=mpeg4        # MPEG-4 Part 2
  --enable-decoder=mpeg2video   # MPEG-2
  --enable-decoder=mpeg1video   # MPEG-1
  --enable-decoder=mjpeg        # Motion JPEG
  --enable-decoder=png          # PNG (for thumbnails)
  --enable-decoder=gif          # GIF
  --enable-decoder=webp         # WebP
  --enable-decoder=theora       # Theora
  --enable-decoder=wmv3         # WMV3
  --enable-decoder=vc1          # VC-1
  --enable-decoder=flv          # FLV

  # ===== 启用解复用器 (封装格式) =====
  --enable-demuxer=mov          # MP4/MOV/M4V
  --enable-demuxer=avi          # AVI
  --enable-demuxer=matroska     # MKV/WebM
  --enable-demuxer=flv          # FLV
  --enable-demuxer=mpegts       # MPEG-TS
  --enable-demuxer=mpegps       # MPEG-PS
  --enable-demuxer=asf          # ASF/WMV
  --enable-demuxer=ogg          # OGG
  --enable-demuxer=gif          # GIF
  --enable-demuxer=image2       # Image sequences

  # ===== 启用解析器 =====
  --enable-parser=h264
  --enable-parser=hevc
  --enable-parser=mpeg4video
  --enable-parser=mpegvideo
  --enable-parser=vp8
  --enable-parser=vp9

  # ===== 启用协议 =====
  --enable-protocol=file        # file:// protocol

  # ===== 启用需要的库 =====
  --enable-swscale              # needed for pixel format conversion (sws_scale)

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

echo "===== Running emconfigure ====="
emconfigure ./configure "${CONFIG_ARGS[@]}"

if [ $? -ne 0 ]; then
    echo "ERROR: configure failed!"
    exit 1
fi

echo "===== Running make ====="
emmake make -j8

if [ $? -ne 0 ]; then
    echo "ERROR: make failed!"
    exit 1
fi

echo "===== Running make install ====="
make install

echo "===== finish build ffmpeg-emcc ====="
