const path = require('path');
const webpack = require('webpack');
const {resolve} = require('path');
const optimization = {
    minimize: false,
};
const moduleConfig = {
    rules: [
        {
            test: /\.(js|ts)$/,
            exclude: /node_modules/,
            use: {
                loader: 'babel-loader',
                options: {
                    babelrc: false,
                    presets: ['@babel/preset-typescript'],
                },
            },
        },
    ],
};
module.exports = [{
    mode: 'production',
    entry: path.join(__dirname, '../src/index'),
    optimization,
    output: {
        filename: 'index.js',
        path: path.resolve(__dirname, '../dist/es'),
        library: 'cheetahCapture',
        libraryTarget: 'umd',
        publicPath: '/',
    },
    resolve: {
        modules: ['node_modules', resolve('src')],
        extensions: ['.ts', '.js'],
    },
    module: moduleConfig,
}, {
    mode: 'production',
    entry: path.join(__dirname, '../src/capture.worker.ts'),
    output: {
        filename: 'capture.worker.js',
        path: path.resolve(__dirname, '../dist/es'),
        publicPath: '/',
    },
    resolve: {
        modules: ['node_modules', resolve('src')],
        extensions: ['.ts', '.js'],
    },
    module: moduleConfig,
    optimization,
}];
