const path = require('path');
const webpack = require('webpack');
const {resolve} = require('path');
const optimization = {
    minimize: false,
};
const moduleConfig = {
    rules: [
        // {
        //     test: /\.worker\.(js|ts)$/,
        //     use: {
        //         loader: 'worker-loader',
        //         options: {
        //             inline: 'no-fallback',
        //         },
        //     },
        // },
        {
            test: /\.(js|ts)$/,
            exclude: /node_modules/,
            use: {
                loader: 'babel-loader',
                options: {
                    presets: [
                        [
                            '@babel/preset-env',
                            {
                                useBuiltIns: 'usage',
                                corejs: 3,
                                modules: false,
                            },
                        ],
                        '@babel/preset-typescript',
                    ],
                },
            },
        },
    ],
};
module.exports = [{
    mode: 'production',
    entry: path.join(__dirname, '../src/index'),
    output: {
        // filename: pathData => {
        //     console.log('==>pathData.filename', pathData.chunk.name);
        //     return pathData.chunk.name === 'main' ? 'index.js' : '[name].[contenthash:6].js';
        // },
        filename: 'index.js',
        path: path.resolve(__dirname, '../dist'),
        library: 'cheetahCapture',
        libraryTarget: 'umd',
        publicPath: '/',
    },
    resolve: {
        modules: ['node_modules', resolve('src')],
        extensions: ['.ts', '.js'],
    },
    module: moduleConfig,
    optimization,
}, {
    mode: 'production',
    entry: path.join(__dirname, '../src/capture.worker.ts'),
    output: {
        // filename: pathData => {
        //     console.log('==>pathData.filename', pathData.chunk.name);
        //     return pathData.chunk.name === 'main' ? 'index.js' : '[name].[contenthash:6].js';
        // },
        filename: 'capture.worker.js',
        path: path.resolve(__dirname, '../dist'),
        // library: 'wasm-capture',
        // libraryTarget: 'umd',
        publicPath: '/',
    },
    resolve: {
        modules: ['node_modules', resolve('src')],
        extensions: ['.ts', '.js'],
    },
    module: moduleConfig,
    optimization,
}];

