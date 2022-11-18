import typescript from '@rollup/plugin-typescript';
import babel from '@rollup/plugin-babel';
import replace from '@rollup/plugin-replace';

export default {
    input: ['src/index.ts', 'src/capture.worker.ts'], // 源文件入口
    output: [
        {
            // file: 'tmp/index.js', // package.json 中 "module": "dist/index.esm.js"
            dir: 'tmp',
            format: 'es', // es module 形式的包， 用来import 导入， 可以tree shaking
            sourcemap: false,
        },
    ],
    plugins: [
        // replace({
        //     preventAssignment: true,
        //     values: {
        //         '__workerBuildUrl__': './capture.worker.js',
        //         // ./capture.worker.js
        //     },
        // }),
        babel({babelHelpers: 'bundled'}),
        typescript(),

    ],
};
