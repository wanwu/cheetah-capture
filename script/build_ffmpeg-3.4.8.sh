#!/bin/bash
###
 # @file: 
 # @Author: jiangzichen01@baidu.com
 # @LastEditors: jiangzichen01@baidu.com
 # @Date: 2022-02-24 15:53:59
### 

echo "===== start build ffmpeg ====="

NOW_PATH=$(cd $(dirname $0); pwd)
echo $NOW_PATH;

WEB_CAPTURE_PATH=$(cd $NOW_PATH/../; pwd)
echo $WEB_CAPTURE_PATH;
FFMPEG_PATH=$(cd $WEB_CAPTURE_PATH/script/ffmpeg-3.4.8; pwd)

rm -rf $WEB_CAPTURE_PATH/lib/ffmpeg-3.4.8

cd $FFMPEG_PATH

./configure --prefix=/Users/mengxiaoxuan/Downloads/web-capture-master/lib/ffmpeg-3.4.8 --disable-shared --enable-static --disable-videotoolbox --disable-audiotoolbox --disable-securetransport --disable-iconv --disable-x86asm

make -j6

make install

echo "===== finish build ffmpeg ====="