/*
 * @file: release.js
 * @Author: jiangzichen01
 * @Date: 2020-12-28 17:33:21
 */
const shell = require('shelljs');

// 实在没招了 写死叭
const username = 'bjh-fe';
const password = 'fe';
const email = 'bjhfe@baidu.com';

function npmPublish() {
    const inputArray = [`${username}\n`, `${password}\n`, `${email}\n`];
    const child = shell.exec('npm login', {async: true});
    child.stdout.on('data', () => {
        const cmd = inputArray.shift();
        if (cmd) {
            child.stdin.write(cmd);
        } else {
            shell.exec('npm publish');
        }
    });
}
npmPublish();
