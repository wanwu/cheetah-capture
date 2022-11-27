interface CMsgRetType {
    width: number;
    height: number;
    duration: number;
    imageDataBuffer: Uint8ClampedArray;
}
class ImageCapture {
    isMKDIR: boolean;
    cCaptureByCount: (info: number, path: string, id: number) => number;
    cCaptureByMs: any;
    captureInfo: Record<number, number>;
    imageList: Record<number, CMsgRetType[]>;
    imgDataPtrList: number[];
    imgBufferPtrList: number[];
    constructor() {
        this.isMKDIR = false;
        this.cCaptureByCount = null;
        this.cCaptureByMs = null; // c的方法
        this.imageList = {};
        this.captureInfo = {};
        this.imgDataPtrList = [];
        this.imgBufferPtrList = [];
    }
    getImageInfo(imgDataPtr): CMsgRetType {
        const width = Module.HEAPU32[imgDataPtr];
        const height = Module.HEAPU32[imgDataPtr + 1];
        const duration = Module.HEAPU32[imgDataPtr + 2];
        const imageBufferPtr = Module.HEAPU32[imgDataPtr + 3];
        const imageBuffer = Module.HEAPU8.slice(imageBufferPtr, imageBufferPtr + width * height * 3);
        //   Module._free(imgDataPtr);
        //   Module._free(imageBufferPtr);
        this.imgDataPtrList.push(imgDataPtr);
        this.imgBufferPtrList.push(imageBufferPtr);

        const imageDataBuffer = new Uint8ClampedArray(width * height * 4);

        let j = 0;
        for (let i = 0; i < imageBuffer.length; i++) {
            if (i && i % 3 === 0) {
                imageDataBuffer[j] = 255;
                j += 1;
            }

            imageDataBuffer[j] = imageBuffer[i];
            j += 1;
        }
        return {
            width,
            height,
            duration,
            imageDataBuffer,
        };
    }

    _singleImage(imgDataPtr) {
        const width = Module.HEAPU32[imgDataPtr];
        const height = Module.HEAPU32[imgDataPtr + 1];
        const duration = Module.HEAPU32[imgDataPtr + 2];
        const imageBufferPtr = Module.HEAPU32[imgDataPtr + 3];
        const imageBuffer = Module.HEAPU8.slice(imageBufferPtr, imageBufferPtr + width * height * 3);
        Module._free(imgDataPtr);
        Module._free(imageBufferPtr);

        const imageDataBuffer = new Uint8ClampedArray(width * height * 4);

        let j = 0;
        for (let i = 0; i < imageBuffer.length; i++) {
            if (i && i % 3 === 0) {
                imageDataBuffer[j] = 255;
                j += 1;
            }

            imageDataBuffer[j] = imageBuffer[i];
            j += 1;
        }
        return {
            width,
            height,
            duration,
            imageDataBuffer,
        };

    }

    getImgList(imgDataPtr, count) {
        const dataArr = [];
        for (let i = 0; i < count; i++) {
            dataArr.push(this._singleImage(imgDataPtr / 4 + i * 4));
        }
        return dataArr;
    }
    // 加载文件
    mountFile(file: File | Blob, MOUNT_DIR: string, id: number) {
        if (!this.isMKDIR) {
            FS.mkdir(MOUNT_DIR);
            this.isMKDIR = true;
        }
        const data: {files?: File[], blobs?: Array<{name: string, data: Blob}>} = {};
        let name: string = '';
        // 判断类型 如果是blob转file
        if (file instanceof File) {
            data.files = [file];
            name = file.name;
        } else {
            name = `${id}.mp4`;
            data.blobs = [{name, data: file}];

        }
        // @ts-ignore
        FS.mount(WORKERFS, data, MOUNT_DIR);
        return name;
    }
    free() {
        // 释放指针内存
        this.imgDataPtrList.forEach(ptr => {
            Module._free(ptr);
        });
        this.imgDataPtrList = [];
        this.imgBufferPtrList.forEach(ptr => {
            Module._free(ptr);
        });
        this.imgBufferPtrList = [];
    }
    capture({id, info, path = '/working', file}) {
        try {
            const name = this.mountFile(file, path, id);
            let retData = 0;
            this.imageList[id] = [];
            if (info instanceof Array) {
                // 说明是按照时间抽
                this.captureInfo[id] = info.length;
                if (!this.cCaptureByMs) {
                    this.cCaptureByMs = Module.cwrap('captureByMs', 'number', ['string', 'string', 'number']);
                }
                // const imgDataPtr =
                retData = this.cCaptureByMs(info.join(','), `${path}/${name}`, id);
                this.free();
            } else {
                this.captureInfo[id] = info;
                if (!this.cCaptureByCount) {
                    this.cCaptureByCount = Module.cwrap('captureByCount', 'number', ['number', 'string', 'number']);
                }
                retData = this.cCaptureByCount(info, `${path}/${name}`, id);
                this.free();
                FS.unmount(path);
                // 完善信息 这里需要一种模式 是否只一次性postmsg 不一张张读取
                if (retData === 0) {
                    throw new Error('Frame draw exception!');
                }
            }
        } catch (e) {
            console.log('Error occurred', e);
            // 如果发生错误 通知
            self.postMessage({
                type: 'receiveError',
                errmsg: e.toString(),
                id,
            });
        }
    }
}

// importScripts('./capture.js');
const imageCapture = new ImageCapture();

let isInit = false;
let angle = 0;
// eslint-disable-next-line @typescript-eslint/no-unused-vars
function transpostFrame(ptr, id) {
    const data = imageCapture.getImageInfo(ptr / 4);
    // push到数组列表
    imageCapture.imageList[id].push({
        ...data,
    });
    self.postMessage({
        type: 'receiveImageOnchange',
        ...data,
        id,
        angle,
    });
    // console.log('transpostFrame==>', id, imageCapture.captureInfo);
    if (imageCapture.imageList[id].length >= imageCapture.captureInfo[id]) {
        // 说明已经到了数目 可以postonfinish事件
        self.postMessage({
            type: 'receiveImageOnSuccess',
            id,
            // ...imageCapture.imageList[id], //TODO: 这个是否post未确定
        });
    }
}
function setAngle(a: string) {
    angle = +a;
}
self.transpostFrame = transpostFrame;
self.setAngle = setAngle;
const initPromise: Promise<URL> = new Promise(res => {
    (self as any).goOnInit = res;
});
self.addEventListener('message', e => {
    // console.log('receivemessage', e.data);
    const {
        type,
        id,
        info,
        path,
        file,
    } = e.data;
    if (type === 'initPath') {
        (self as any).goOnInit(info);
    }
    if (isInit && type === 'startCapture') {
        imageCapture.capture({
            id,
            info,
            path,
            file,
        });
    }
});


(self as any).Module = {
    instantiateWasm: async (info, receiveInstance) => {
        const url = await initPromise;
        fetch(url || './capture.worker.wasm').then(response => response.arrayBuffer())
            .then(bytes => WebAssembly.instantiate(bytes, info))
            .then(instance => receiveInstance(instance.instance));
        // WebAssembly.instantiate(bytes, info).then(result => {
        //     receiveInstance(result.instance);
        // });
    },
    onRuntimeInitialized: () => {
        isInit = true;
        // console.log('wasm success');
        self.postMessage({
            type: 'init',
            data: {},
        });
    },
};
