# cheetah-capture

<div>
<img src="http://bj.bcebos.com/baidu-rmb-video-cover-1/goods/2022-11/1668701480293/dd3a15fcc40b.png" height="60" alt="cheetah" />
</div>
基于自定义编译ffmpeg的截帧工具

## 依赖库&编译工具

* ffmpeg-3.4.8
* emscripten

## 支持编码

* H.264
* H.265
* Mpeg2
* Mpeg4
* VP8
* VP9

## 支持格式

涵盖主流格式。

* Mkv
* Mov
* Avi
* Mp4
* Webm

## 准备工作

* 编译前需要下载 [ffmpeg-3.4.8.tar.xz](http://ffmpeg.org/releases/ffmpeg-3.4.8.tar.xz) 并解压至 `script` 目录下方。

* emcc下载:git clone https://github.com/emscripten-core/emsdk.git

```shell
  git pull
  # Download and install the latest SDK tools.
  ./emsdk install latest
  # Make the "latest" SDK "active" for the current user. (writes .emscripten file)
  ./emsdk activate latest
   # Activate PATH and other environment variables in the current terminal
   source ./emsdk_env.sh
```

* `emsdk` 安装目录需要与 `代码库` 同级

## 编译脚本

`script`下面是编译脚本
`build_ffmpeg-3.4.8.sh` 编译本地c编译环境的ffmpeg的lib库
`build-emcc.sh` 编译emcc环境需要的ffmpeg的lib库，编译结果lib/ffmpeg-emcc
`build_wasm.sh` 使用ems编译wasm 和 glue code & worker code
`build.sh` 同上以及生成dts类型

## demo运行

demo文件夹下`index.html` 使用`live server`启动即可，`live-server --port=5501`

## 调试环境准备

在已经完成准备工作的前提

- 若进行c环境调试 scripts目录下执行`sh build_ffmpeg-3.4.8.sh`编译你的对应环境的lib，`sh reload.sh`执行capture.c 文件。
- 进行js环境调试，先执行`build-emcc.sh`生产emcc环境需要的ffmpeg的lib库，如若已有库直接执行 `sh build-wasm.sh`，包含`npm run webpack-capture`以及`emcc`编译命令，前者生成`index.js`以及`capture.worker.js`，后者生成`capture.worker.js`(包含emcc的胶水代码) 和`capture.worker.wasm`。
- html环境引入，直接引入`index.js`即可，umd变量`cheetahCapture`。

## 关于版本控制

版本的hash不会在类库层面去做 倾向于业务方使用打包工具来做。

## 提交及发包

本地提交`npm run commit`
更新并写入changelog`npm run release`

## api

`initCapture`：`({workerPath, wasmPath}) => Promise<capture>` 初始化worker环境，拉取wasm，返回capture方法
接受参数如下
|  参数   | 类型  | 含义  | 是否必须  |
|  ----  | ----  |----  |----  |
| workerPath  | URL | string | woker路径，node_modules/tmp/capture.worker.js  | y  |
| wasmPath  | URL | string | wasm路径，node_modules/tmp/capture.worker.wasm | y |

`capture`: `(args) => void` 在worker里执行截帧方法
接受参数如下
|  参数   | 类型  |含义  | 是否必须  |
|  ----  | ----  |----  |----  |
| info  | number[] / number |  传递number是按照数目抽帧，传递数组是指定抽帧的时间 | y |
| path  | string | workerfs建立文件目录 | n |
| file  | File / blob |  文件  | y |
| onChange  | (prev: PrevType, now: nowType, info: {width: number, height: number, duration: number}) => void | 当抽帧结果变化的回调  | n |
| onSuccess  | (prev: PrevType) => void  | 当抽帧结束并成功的回调  | n |
| onError  | (errmeg: string) => void | 当抽帧过程出现错误的回调  | n |

例子可以参考 `demo/index.html`。