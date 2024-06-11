const typescript = require('@rollup/plugin-typescript');
const babel = require('@rollup/plugin-babel');

module.exports = {
    input: ['src/index.ts', 'src/capture.worker.ts'], // 源文件入口
    output: [
        {
            // file: 'dist/index.js', // package.json 中 "module": "dist/index.esm.js"
            dir: 'dist',
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
