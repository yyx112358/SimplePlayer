echo "Build FFMpeg"

echo ${FF_SOURCE_DIR:-"../FFMpeg"}
echo ${FF_BUILD_DIR:-"FFMpeg"}

rm -rf $FF_BUILD_DIR

# 编译FFMpeg
cd $FF_SOURCE_DIR
./configure --enable-debug --enable-ffplay --enable-nonfree --enable-gpl
make -j

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