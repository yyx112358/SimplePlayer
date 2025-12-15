echo "Build FFMpeg"

# 任何步骤失败时，立即退出脚本并显示错误信息
set -euo pipefail
trap 'echo "FFMpeg build failed, aborting." >&2' ERR

echo ${FF_SOURCE_DIR:-"../FFMpeg"}   # 源码路径
echo ${FF_BUILD_DIR:-"FFMpeg"}       # 产物输出路径

rm -rf $FF_BUILD_DIR  # 重置输出目录

# 编译FFMpeg
cd $FF_SOURCE_DIR
./configure --enable-debug --enable-ffplay --enable-nonfree --enable-gpl  # 配置FFmpeg
make -j"$(sysctl -n hw.ncpu)"  # 与本机CPU核心数匹配的并行编译

# 复制header和lib
libnames=("libavcodec" "libavdevice" "libavfilter" "libavformat" "libavutil" "libpostproc" "libswresample" "libswscale")
cd $FF_BUILD_DIR
CUR_DIR=`pwd`
for libname in "${libnames[@]}"
do
    echo "Copy $CUR_DIR/$FF_SOURCE_DIR/$libname/*.h ---> $CUR_DIR/$FF_BUILD_DIR/include/$libname/"
    mkdir -p $FF_BUILD_DIR/include/$libname
    cp -r $FF_SOURCE_DIR/$libname/*.h $FF_BUILD_DIR/include/$libname/
    
    echo "Copy $CUR_DIR/$FF_SOURCE_DIR/$libname/*.a ---> $FF_BUILD_DIR/lib/"
    mkdir -p $FF_BUILD_DIR/lib
    cp $FF_SOURCE_DIR/$libname/*.a $FF_BUILD_DIR/lib/
done

# 清除中间文件
cd $FF_SOURCE_DIR
make clean
