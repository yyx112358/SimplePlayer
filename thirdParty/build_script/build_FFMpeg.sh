echo "Build FFMpeg"

SOURCE_DIR="../FFMpeg"
BUILD_DIR="FFMpeg"

cd ../build
rm -rf $BUILD_DIR

# 编译FFMpeg
cd $SOURCE_DIR
# ./configure --enable-debug --enable-ffplay --enable-nonfree --enable-gpl
# make -j10

# 复制header和lib
libnames=("libavcodec" "libavdevice" "libavfilter" "libavformat" "libavutil" "libpostproc" "libswresample" "libswscale")
cd ../build
CUR_DIR=`pwd`
for libname in "${libnames[@]}"
do
    echo "Copy $CUR_DIR/$SOURCE_DIR/$libname/*.h ---> $CUR_DIR/$BUILD_DIR/include/$libname/"
    mkdir -p $BUILD_DIR/include/$libname
    cp -r $SOURCE_DIR/$libname/*.h $BUILD_DIR/include/$libname/
    
    echo "Copy $CUR_DIR/$SOURCE_DIR/$libname/*.a ---> $CUR_DIR/$BUILD_DIR/lib/"
    mkdir -p $BUILD_DIR/lib
    cp $SOURCE_DIR/$libname/*.a $BUILD_DIR/lib/
done

make clean