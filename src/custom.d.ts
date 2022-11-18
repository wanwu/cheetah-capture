// / <reference types="emscripten" />
declare module '*.worker.ts' {
    class CaptureWorker extends Worker {
        constructor();
    }

    export default CaptureWorker;
  }
interface ModuleType extends EmscriptenModule {
    cwrap: typeof cwrap;
}
declare const Module: ModuleType;
interface self {
    Module: ModuleTypes;
    goOnInit: (url: URL | string) => void;
}
// interface WORKERFS {
//     DIR_MODE: number;
//     FILE_MODE: number;
//     reader: unknown;
//     mount: unknown;
// }
