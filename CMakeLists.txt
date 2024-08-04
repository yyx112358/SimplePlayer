cmake_minimum_required(VERSION 3.0)
project(SimplePlayer)

# 1.环境配置

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

# 检查当前操作系统
if(WIN32)
    message("This is Windows.")
    
    # 设置C++编译器
    # set(CMAKE_CXX_COMPILER "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.34.31933/bin/Hostx64/x64/cl.exe")

elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    message("This is macOS.")
    
    # 设置C++编译器
    set(CMAKE_CXX_COMPILER "clang++")
else()
    message("This is not Windows or macOS.")
endif()

# 2. 添加依赖库 
# 2.1 spdlog日志库
add_subdirectory(thirdParty/spdlog)

# 2.2 glm矩阵计算
add_subdirectory(thirdParty/glm)

# 2.3 FFMpeg
if(WIN32)
    # 查找是否有VCPKG编译的FFMPEG
    set(VCPKG_FFMPEG_INSTALL_DIR "${PROJECT_SOURCE_DIR}/build/vcpkg/packages/ffmpeg_x64-windows-static")
    list(APPEND CMAKE_MODULE_PATH "${VCPKG_FFMPEG_INSTALL_DIR}/share/ffmpeg")
    find_package(FFMPEG)
    # 找不到，则调用build_FFMpeg.ps1编译ffmpeg，再查找一次
    if (NOT FFMPEG_FOUND)
        message("找不到${VCPKG_FFMPEG_INSTALL_DIR}，执行build_FFMpeg.ps1脚本...")
        execute_process(COMMAND powershell -File "${PROJECT_SOURCE_DIR}/thirdParty/build_script/build_FFMpeg.ps1")
        find_package(FFMPEG REQUIRED) # 再找一次
    endif()
    message("-- FFMpeg: include:${FFMPEG_INCLUDE_DIRS} lib:${FFMPEG_LIBRARY_DIRS}")
else()

endif()


# 3. 添加源文件
include_directories(${PROJECT_SOURCE_DIR}/proj/win/GUI/imgui)
include_directories(${PROJECT_SOURCE_DIR}/thirdParty/)    
include_directories(${FFMPEG_INCLUDE_DIRS})

# 收集当前目录下的所有cpp文件
file(GLOB_RECURSE DEMO_FILES 
    proj/win/GUI/*.cpp 
    proj/win/GUI/imgui/*.cpp
    proj/win/main.cpp)


# 添加可执行文件并将所有cpp文件链接起来
add_executable(${PROJECT_NAME} ${DEMO_FILES})


# 4. 链接
target_link_libraries(${PROJECT_NAME} 
    spdlog 
    OpenGL32.lib
    ${FFMPEG_LIBRARIES})