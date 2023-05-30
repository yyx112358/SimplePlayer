# 清空build
if [ -d build ]; then
    rm -rf build/*
    echo "已清空build文件夹"
else
    mkdir -p build
    echo "已创建空build文件夹"
fi

# cmake创建工程
cd build
cmake -G Xcode ..

# 启动工程
open SimplePlayer.xcodeproj/
