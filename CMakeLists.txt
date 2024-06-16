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
# spdlog日志库
add_subdirectory(thirdParty/spdlog)

# glm矩阵计算
add_subdirectory(thirdParty/glm)


# 3. 添加源文件

include_directories(${PROJECT_SOURCE_DIR}/proj/win/GUI/imgui)

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
    OpenGL32.lib)