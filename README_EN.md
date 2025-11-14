# cheetah-capture

English | [简体中文](./README.md)

<div>
<img src="https://pic.rmb.bdstatic.com/activity/2025-11/1762943255629/692840b3bee6.png" height="60" alt="cheetah" />
</div>

Video frame extraction tool based on custom-compiled FFmpeg, supporting mainstream formats like Mp4, Mov, Avi, Webm, Mkv, etc.

## API

> Overall workflow: Call `initCapture` method with worker and wasm paths, it returns a cheetahCapture object. Use methods on cheetahCapture to mount files, extract frames, get metadata, etc. Call free to release resources when done.

`initCapture`: `({workerPath, wasmPath}) => Promise<capture>` Initialize worker environment, fetch wasm, and return capture method

Parameters:

| Parameter | Type | Description | Required |
| ---- | ---- | ---- | ---- |
| workerPath | URL / string | Worker path, e.g., node_modules/dist/capture.worker.js. Due to worker same-origin restrictions, you can pass a BlobUrl to resolve | y |
| wasmPath | URL / string | Wasm path, e.g., node_modules/dist/capture.worker.wasm | y |

`mountFile`: `(file: File) => string` Mount file, returns `fileName`, parameters:

| Parameter | Type | Description | Required |
| ---- | ---- | ---- | ---- |
| path | string | WorkerFS file directory | n, default `/working` |
| file | File / blob | File | y |

For compatibility with V0.1.x, the `capture` method can be used directly without calling mountFile by passing mountFile parameters. If you choose this approach, we will actively manage your lifecycle and perform memory cleanup for you.

`capture`: `(args) => void` Execute frame extraction in worker

Parameters:

| Parameter | Type | Description | Required |
| ---- | ---- | ---- | ---- |
| info | number[] / number | Pass number to extract by count, pass array to specify extraction times | y |
| path | string | WorkerFS file directory | n |
| file | File / blob | File | n, required in v0.1 |
| onChange | (prev: PrevType, now: nowType, info: {width: number, height: number, duration: number}) => void | Callback when frame extraction result changes | n |
| onSuccess | (prev: PrevType) => void | Callback when frame extraction completes successfully | n |
| onError | (errmeg: string) => void | Callback when errors occur during extraction | n |

`getMetadata`: `(args: {info: string; })=> void` Get video metadata, parameters:

| Parameter | Type | Description | Required |
| ---- | ---- | ---- | ---- |
| info | string | Key of metadata to retrieve | y |
| onSuccess | (args: {meta: string}) => void | Success callback, executes regardless of key existence, returns empty string if not found | n |

`hasAudioTrack`: `(args)=> void` Check if video contains audio track. Must call `mountFile` first to mount the file. Parameters:

| Parameter | Type | Description | Required |
| ---- | ---- | ---- | ---- |
| onSuccess | (hasAudio: boolean) => void | Success callback, true means contains audio, false means no audio (silent video) | n |
| onError | (errmeg: string) => void | Error callback | n |

`free`: `(args)=> void` Release file storage space. Note: only releases files, not directories. Parameters:

| Parameter | Type | Description | Required |
| ---- | ---- | ---- | ---- |
| onSuccess | () => void | Success callback | n |

`terminate`: `() => void` Terminate the worker and release all resources (including WASM memory). Recommended to call after completing all operations.

* See `demo/index.html` for usage examples.

## Dependencies & Build Tools

* ffmpeg-3.4.8
* emscripten

## Preparation

* Before compilation, download [ffmpeg-3.4.8.tar.xz](http://ffmpeg.org/releases/ffmpeg-3.4.8.tar.xz) and extract it to the `script` directory.

* Download emsdk: `git clone https://github.com/emscripten-core/emsdk.git`
* Install emsdk, execute in the emsdk directory:

```shell
git pull
# Switch to compatible version
git checkout 3d6d8ee910466516a53e665b86458faa81dae9ba
# Download and install the latest SDK tools.
./emsdk install latest
# Make the "latest" SDK "active" for the current user. (writes .emscripten file)
./emsdk activate latest
# Activate PATH and other environment variables in the current terminal
source ./emsdk_env.sh
```

* `emsdk` needs to be installed in the repository root directory

## Build Scripts

* Install dependencies: `npm i`, then execute scripts in the script directory

Scripts under `script`:

* `build_ffmpeg-3.4.8.sh` - Compile FFmpeg lib for local C compilation environment
* `build-emcc.sh` - Compile FFmpeg lib for emcc environment, output to lib/ffmpeg-emcc
* `build_wasm.sh` - Compile wasm and glue code & worker code using ems
* `build.sh` - Same as above plus generate dts types

If you only modified C code and TypeScript code, just run `sh build_wasm.sh`.

## Running Demo

Start `index.html` in the demo folder using `live server`: `live-server --port=5501`

## Debug Environment Setup

Prerequisites: Complete the preparation work

- For C environment debugging: Execute `sh build_ffmpeg-3.4.8.sh` in the scripts directory to compile libs for your environment, then `sh reload.sh` to execute the capture.c file.
- For JS environment debugging: First execute `build-emcc.sh` to generate FFmpeg libs for emcc environment. If libs already exist, directly execute `sh build-wasm.sh`, which includes `npm run webpack-capture` and `emcc` compilation commands. The former generates `index.js` and `capture.worker.js`, the latter generates `capture.worker.js` (with emcc glue code) and `capture.worker.wasm`.
- For HTML environment: Just include `index.js`, UMD variable `cheetahCapture`.

## Version Control

Version hashing is not handled at the library level and is expected to be done by the business side using build tools.

## Commit and Release

Local commit: `npm run commit`
Update and write changelog: `npm run release`

