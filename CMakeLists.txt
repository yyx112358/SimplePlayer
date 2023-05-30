cmake_minimum_required(VERSION 3.0)
project(SimplePlayer)

# 设置C++编译器
set(CMAKE_CXX_COMPILER "clang++")

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)

# 收集当前目录下的所有cpp文件
file(GLOB SDK_FILES src/*)

# 添加可执行文件并将所有cpp文件链接起来
add_executable(${PROJECT_NAME} ${SDK_FILES})

