message("开始引入FFMPEG...")
set(FFMPEG_FOUND FALSE)

if(WIN32) # windows使用vcpkg来编译和获取
    # 设置相关目录
    set(VCPKG_FFMPEG_INSTALL_DIR "${PROJECT_SOURCE_DIR}/thirdParty/build/vcpkg/packages/ffmpeg_x64-windows-static")
    set(FFMPEG_INCLUDE_DIRS "${VCPKG_FFMPEG_INSTALL_DIR}/include")
    set(FFMPEG_LIBRARY_DIRS "${VCPKG_FFMPEG_INSTALL_DIR}/lib")

    if (NOT EXISTS VCPKG_FFMPEG_INSTALL_DIR)
        message("找不到${VCPKG_FFMPEG_INSTALL_DIR}，执行build_FFMpeg.ps1脚本...")
        execute_process(
            COMMAND powershell -File "build_FFMpeg.ps1"
        )
    endif()


    
    # if (EXISTS FFMPEG_INCLUDE_DIRS AND EXISTS FFMPEG_INCLUDE_DIRS AND EXISTS FFMPEG_LIBRARY_DIRS
    #     AND EXISTS "${FFMPEG_INCLUDE_DIRS}/libavcodec/avcodec.h" AND EXISTS "${FFMPEG_LIBRARY_DIRS}/av")

    # message("引入FFMpeg ${VCPKG_FFMPEG_INSTALL_DIR}")



    # file(GLOB_RECURSE FFMPEG_LIBRARIES ${FFMPEG_LIBRARY_DIRS}/*.lib)
    
    # message("引入FFMpeg ${FFMPEG_LIBRARIES}")
endif()