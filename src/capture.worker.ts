import {Events} from './consts';
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
    name: string;
    path: string;
    file: File | Blob;

    constructor() {
        this.isMKDIR = false;
        this.cCaptureByCount = null;
        this.cCaptureByMs = null; // c的方法
        this.imageList = {};
        this.captureInfo = {};
        this.imgDataPtrList = [];
        this.imgBufferPtrList = [];
        this.name = '';
        this.file = null;
        this.path = '/working';
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
    mountFile(file: File | Blob, MOUNT_DIR: string = this.path, id: number) {
        // 防止重复mount dir
        if (!this.isMKDIR) {
            FS.mkdir(MOUNT_DIR);
            this.isMKDIR = true;
        }
        this.file = file;
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
        this.name = name;
        this.path = MOUNT_DIR;
        self.postMessage({
            type: Events.mountFileSuccess,
            id,
        });
        return name;
    }
    free({id}) {
        // 释放指针内存
        this.imgDataPtrList.forEach(ptr => {
            Module._free(ptr);
        });
        this.imgDataPtrList = [];
        this.imgBufferPtrList.forEach(ptr => {
            Module._free(ptr);
        });
        this.imgBufferPtrList = [];
        // 释放文件
        FS.unmount(this.path);
        // 清除副作用
        this.name = '';
        this.file = null;
        this.path = '/working';
        self.postMessage({
            type: Events.freeOnSuccess,
            id,
        });
    }
    capture({id, info, path = this.path, file = this.file}) {
        let isOnce = false;
        this.path = path;
        try {
            if (!this.name) {
                isOnce = true;
                this.name = this.mountFile(file, path, id);
            }
            let retData = 0;
            this.imageList[id] = [];
            if (info instanceof Array) {
                // 说明是按照时间抽
                this.captureInfo[id] = info.length;
                if (!this.cCaptureByMs) {
                    this.cCaptureByMs = Module.cwrap('captureByMs', 'number', ['string', 'string', 'number']);
                }
                retData = this.cCaptureByMs(info.join(','), `${path}/${this.name}`, id);
            } else {
                this.captureInfo[id] = info;
                if (!this.cCaptureByCount) {
                    this.cCaptureByCount = Module.cwrap('captureByCount', 'number', ['number', 'string', 'number']);
                }
                retData = this.cCaptureByCount(info, `${path}/${this.name}`, id);
                // 完善信息 这里需要一种模式 是否只一次性postmsg 不一张张读取
                if (retData === 0) {
                    this.free({id});
                    throw new Error('Frame draw exception!');
                }
            }
            if (isOnce) {
                this.free({id});
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

    getMetadata(key: string, id: number) {
        // getMetadata(key, path, id)
        if (!this.name) {
            throw new Error('Please mount file first!');
        }
        const cGetMetadata = Module.cwrap('getMetaDataByKey', 'string', ['string', 'string', 'number']);
        const metadataValue = cGetMetadata(key, `${this.path}/${this.name}`, id);
        // cGetMetadata();

        self.postMessage({
            type: Events.getMetadataOnSuccess,
            meta: metadataValue,
            id,
        });
    }
}

// importScripts('./capture.js');
const imageCapture = new ImageCapture();

let isInit = false;
const metaDataMap = {};

self.addEventListener('message', e => {
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

    if (type === 'mountFile') {
        imageCapture.mountFile(file, path, id);
    }

    if (isInit && type === 'startCapture') {
        metaDataMap[id] = {};
        imageCapture.capture({
            id,
            info,
            path,
            file,
        });
    }

    if (type === Events.getMetadata) {
        imageCapture.getMetadata(info, id);
    }

    if (type === Events.free) {
        imageCapture.free({id});
    }
});


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
        meta: metaDataMap[id] || {},
    });
    if (imageCapture.imageList[id].length >= imageCapture.captureInfo[id]) {
        // 说明已经到了数目 可以postonfinish事件
        self.postMessage({
            type: 'receiveImageOnSuccess',
            id,
            meta: metaDataMap[id] || {},
            // ...imageCapture.imageList[id], //TODO: 这个是否post未确定
        });
    }
}
function setAngle(a: string, id: number) {
    metaDataMap[id].angle = +a;
}
function setDescription(a: string, id: number) {
    metaDataMap[id].description = a;
}
self.transpostFrame = transpostFrame;
self.setAngle = setAngle;
self.setDescription = setDescription;
const initPromise: Promise<URL> = new Promise(res => {
    (self as any).goOnInit = res;
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
        self.postMessage({
            type: 'init',
            data: {},
        });
    },
};
