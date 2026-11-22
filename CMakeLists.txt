cmake_minimum_required(VERSION 3.10)

# 设置交叉编译依赖文件
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/rk3506_toolchain.cmake)

# 设置工程名称
project(Test)

find_package(CURL REQUIRED)
find_package(ALSA REQUIRED)

# 设置生成的可执行文件名字
set(BIN_NAME "aichat")

# 添加可执行文件
add_executable(${BIN_NAME})

# 添加编译选项
target_compile_options(${BIN_NAME} PRIVATE -Wall -Wextra)

file(GLOB_RECURSE SRC src/*.c)

# 添加可执行文件依赖的源文件
target_sources(${BIN_NAME}
    PRIVATE
    ${SRC}
)

# 添加头文件路径
target_include_directories(${BIN_NAME}
    PRIVATE
    include/
    third_library/include
)

target_link_directories(${BIN_NAME}
    PRIVATE
    ${CMAKE_SOURCE_DIR}/third_library/lib
)

# 添加编译链接库
target_link_libraries(${BIN_NAME}
    PRIVATE
    CURL::libcurl
    ALSA::ALSA
    json-c
    zlog
    m
    resolv
)
