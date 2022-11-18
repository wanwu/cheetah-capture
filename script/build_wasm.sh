#!/bin/bash

echo "===== start build wasm ====="

NOW_PATH=$(cd $(dirname $0); pwd)

WEB_CAPTURE_PATH=$(cd $NOW_PATH/../; pwd)

FFMPEG_PATH=$(cd $WEB_CAPTURE_PATH/lib/ffmpeg-emcc; pwd)

TOTAL_MEMORY=33554432

# source $WEB_CAPTURE_PATH/../emsdk/emsdk_env.sh
# rm -rf ./demo/capture.wasm
    # --pre-js $WEB_CAPTURE_PATH/src/worker.js \

npm run webpack-capture
emcc $WEB_CAPTURE_PATH/src/capture.c $FFMPEG_PATH/lib/libavformat.a $FFMPEG_PATH/lib/libavcodec.a $FFMPEG_PATH/lib/libswscale.a $FFMPEG_PATH/lib/libavutil.a \
    -O0 \
    -lworkerfs.js \
    --pre-js $WEB_CAPTURE_PATH/tmp/capture.worker.js \
    -I "$FFMPEG_PATH/include" \
    -s WASM=1 \
    -s TOTAL_MEMORY=$TOTAL_MEMORY \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
    -s EXPORTED_FUNCTIONS='["_main", "_free", "_captureByMs", "_captureByCount"]' \
    -s ASSERTIONS=0 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -o $WEB_CAPTURE_PATH/tmp/capture.worker.js
# cp -r ./tmp/capture.wasm ./demo/capture.wasm
# cp -r ./tmp/capture.js ./demo/capture.js
# npx uglifyjs $WEB_CAPTURE_PATH/tmp/capture.js -o $WEB_CAPTURE_PATH/tmp/capture.js

echo "===== finish build wasm ====="
