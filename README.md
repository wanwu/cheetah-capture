# cheetah-capture

<div>
<img src="http://bj.bcebos.com/baidu-rmb-video-cover-1/goods/2022-11/1668701480293/dd3a15fcc40b.png" height="60" alt="cheetah" />
</div>
基于自定义编译ffmpeg的截帧工具，支持Mp4、Mov、Avi、Webm、Mkv等主流格式。

## API
> 整体调用流程，调用`initCapture`方法，传入worker和wasm路径，返回cheetahCapture对象，调用cheetahCapture上的方法进行mount文件、抽帧、获取元数据等操作，结束后调用free进行释放。

`initCapture`：`({workerPath, wasmPath}) => Promise<capture>` 初始化worker环境，拉取wasm，返回capture方法
接受参数如下
|  参数   | 类型  | 含义  | 是否必须  |
|  ----  | ----  |----  |----  |
| workerPath  | URL / string | woker路径，eg:node_modules/dist/capture.worker.js，因为有woker有同源限制，你可以传递BlobUrl来解决  | y  |
| wasmPath  | URL / string | wasm路径，eg:node_modules/dist/capture.worker.wasm | y |

`mountFile`: `(file: File) => string` 挂载文件，返回mountFile的`fileName`，接受参数如下
接受参数如下
|  参数   | 类型  |含义  | 是否必须  |
|  ----  | ----  |----  |----  |
| path  | string | workerfs建立文件目录 | n，默认`/working` |
| file  | File / blob |  文件  | y |

为了兼容V0.1.x版本，`capture`方法兼容不调用mountFile可以直接使用，传入mountFile需要的参数，会在capture内部进行mountFile，如果您选择这种方式我们将会主动接管你的生命周期，为你进行内存释放操作。
`capture`: `(args) => void` 在worker里执行截帧方法
接受参数如下
|  参数   | 类型  |含义  | 是否必须  |
|  ----  | ----  |----  |----  |
| info  | number[] / number |  传递number是按照数目抽帧，传递数组是指定抽帧的时间 | y |
| path  | string | workerfs建立文件目录 | n |
| file  | File / blob |  文件  | n，v0.1必须 |
| onChange  | (prev: PrevType, now: nowType, info: {width: number, height: number, duration: number}) => void | 当抽帧结果变化的回调  | n |
| onSuccess  | (prev: PrevType) => void  | 当抽帧结束并成功的回调  | n |
| onError  | (errmeg: string) => void | 当抽帧过程出现错误的回调  | n |

`getMetadata: (args: {info: string; })=> void` 获取视频元数据，具体args参数如下
|  参数   | 类型  |含义  | 是否必须  |
|  ----  | ----  |----  |----  |
| info  | string |  要获取的元数据的key | y |
| onSuccess  | (args: {meta: string}) => void  | 读取成功的回调，无论是否有该key都会执行，没有返回的空字符串  | n |

* 使用例子可以参考 `demo/index.html`。

## 依赖库&编译工具

* ffmpeg-3.4.8
* emscripten


## 准备工作

* 编译前需要下载 [ffmpeg-3.4.8.tar.xz](http://ffmpeg.org/releases/ffmpeg-3.4.8.tar.xz) 并解压至 `script` 目录下方。

* emcc下载:git clone https://github.com/emscripten-core/emsdk.git
* 安装emsdk，在emsdk目录下执行

```shell
  git pull
  # Download and install the latest SDK tools.
  ./emsdk install latest
  # Make the "latest" SDK "active" for the current user. (writes .emscripten file)
  ./emsdk activate latest
   # Activate PATH and other environment variables in the current terminal
   source ./emsdk_env.sh
```

* `emsdk` 需要安装于代码库根目录

## 编译脚本
* 安装依赖 `npm i`，执行script目录下的脚本
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

