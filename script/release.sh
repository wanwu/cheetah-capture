# 发包相关
set -e
export PATH=$NODEJS_14_15_4_BIN:$PATH
npm config set registry http://registry.npm.baidu-int.com

npm install shelljs
node ./release.js
