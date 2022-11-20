import {Events} from './consts';
let captureWorker: null | Worker = null;
function workerPost(info: {type: string, [key: string]: unknown}) {
    captureWorker && captureWorker.postMessage({
        ...info,
    });
}
async function initWorker(url: URL|string, wasmPath: URL|string): Promise<Worker> {
    if (captureWorker) {
        return captureWorker;
    }
    // captureWorker = new Worker(new URL('./capture.worker.js', import.meta.url));
    captureWorker = new Worker(url);
    workerPost({
        type: 'initPath',
        info: wasmPath.toString(),
    });
    const promise = new Promise<Worker>(resolve => {
        captureWorker && captureWorker.addEventListener('message', e => {
            if (e?.data?.type === 'init') {
                // wasm初始化完毕
                resolve(captureWorker);
            }
        });
    });
    return promise;
}
interface PrevType {
    url: string[];
    blob?: Blob[];
}
interface nowType {
    url: string;
    blob?: Blob;
}

interface CallbackType {
    onChange?: (prev: PrevType, now: nowType, info: {width: number, height: number, duration: number}) => void;
    onSuccess?: (prev: PrevType) => void;
    onError?: (errmeg: string) => void;
}
interface MapInfoType extends CallbackType{
    url: string[];
}
interface CaptureInfo extends CallbackType{
    info: number[] | number;
    path?: string;
    file: File;
    returnType?: 'blob' | 'base64'; // 默认blob
}
function createRequest() {
    let currentId = 0;
    const map: Map<number, MapInfoType> = new Map();
    return {
        // 获取视频唯一id
        setFrameCallback(item: CallbackType) {
            const id = ++currentId;
            map.set(currentId, {
                ...item,
                url: [],
            });
            return id;
        },
        // 设置
        getCbk(idx: number) {
            return map.get(idx);
        },
    };
}
const pool = createRequest();
const canvas = document.createElement('canvas');
const ctx = canvas.getContext('2d')!;

async function getUrl(width: number, height: number, imageDataBuffer):
Promise<{url: string, blob?: Blob}> {
    canvas.width = width;
    canvas.height = height;
    const imageData = new ImageData(imageDataBuffer, width, height);
    ctx.putImageData(imageData, 0, 0, 0, 0, width, height);
    // const blob = new Blob([imageDataBuffer.buffer], {type: 'image/png'} /* (1) */);
    return {
        url: canvas.toDataURL('image/jpeg'),
        // blob: blob,
    };
    // return new Promise(resolve => {
    //     canvas.toBlob(blob => {
    //         const url = URL.createObjectURL(blob);
    //         resolve({
    //             url,
    //             blob,
    //         });
    //     });
    // });
}

function startCapture(id: number, info: CaptureInfo['info'], path: CaptureInfo['path'], file: CaptureInfo['file']) {
    workerPost({
        type: Events.startCapture,
        id,
        info,
        path,
        file,
    });
}
function capture(data: CaptureInfo) {
    const {info, path, file, ...func} = data;
    const id = pool.setFrameCallback(func);
    startCapture(id, info, path, file);
}
export async function initCapture({
    workerPath,
    wasmPath,
}: {
    workerPath: URL | string;
    wasmPath: URL | string;
}) {
    const worker = await initWorker(workerPath, wasmPath);
    worker.addEventListener('message', async e => {
        switch (e?.data?.type) {
            case Events.receiveImageOnchange: {
                const {imageDataBuffer, width, height, duration, id} = e.data || {};
                const img = await getUrl(width, height, imageDataBuffer);
                const cbk = pool.getCbk(id);
                const {onChange} = cbk;
                const info = {width, height, duration: duration / 1000000};
                const {url} = pool.getCbk(id);
                onChange && onChange({url}, img, info);
                url.push(img.url);
                break;
            }
            case Events.receiveImageOnSuccess: {
                const {id} = e.data || {};
                const cbk = pool.getCbk(id);
                const {onSuccess} = cbk;
                const {url} = pool.getCbk(id);
                onSuccess && onSuccess({url});
                break;
            }
            case Events.receiveError: {
                const {errmsg, id} = e.data || {};
                const cbk = pool.getCbk(id);
                const {onError} = cbk;
                onError && onError(errmsg);
                break;
            }
            default:
                break;
        }
    });
    return {
        capture,
    };
}

export default initCapture;
