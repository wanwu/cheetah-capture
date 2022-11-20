set -e
export PATH=$NODEJS_16_10_0_BIN:$PATH

echo "node: $(node -v)"
echo "npm: v$(npm -v)"

echo "now compiling $1"

mkdir -p output/dist
mkdir -p output/types
# 贴dist目录 构造结构
cp -r dist/*  output/dist
cp -r types/*  output/dist
cp -r package.json  output/
cp -r README.md  output/

fi

echo "build end"
