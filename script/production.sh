set -e
###
 # @file: 
 # @Author: jiangzichen01@baidu.com
 # @LastEditors: jiangzichen01@baidu.com
 # @Date: 2022-09-14 18:30:43
### 
export PATH=$NODEJS_16_10_0_BIN:$PATH

echo "node: $(node -v)"
echo "npm: v$(npm -v)"

echo "now compiling $1"

mkdir -p output/tmp
mkdir -p output/types
# 贴tmp目录 构造结构
cp -r tmp/*  output/tmp
cp -r types/*  output/tmp
cp -r package.json  output/
cp -r README.md  output/

fi

echo "build end"
