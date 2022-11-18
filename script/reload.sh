# gcc本地测试c文件的命令
rm -rf ./cap
gcc ../src/capture2.c -L ../lib/ffmpeg-3.4.8/lib -lavcodec  -lavdevice -lavfilter -lavformat -llzma -lavutil -lswresample -lswscale -I lib/ffmpeg-3.4.8/include -lbz2  -lz -o cap -Wall

./cap