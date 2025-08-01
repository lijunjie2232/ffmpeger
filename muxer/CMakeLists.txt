cmake_minimum_required(VERSION 3.12)
project(avmerger)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 查找Python
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# 查找pybind11
find_package(pybind11 CONFIG REQUIRED)

# 设置FFmpeg路径 - 请根据您的实际路径修改
set(FFMPEG_ROOT "C:\Program Files\ffmpeg" CACHE STRING "Path to FFmpeg root directory")

# 设置FFmpeg包含目录
set(FFMPEG_INCLUDE_DIRS 
    ${FFMPEG_ROOT}/include
)

# 设置FFmpeg库目录
set(FFMPEG_LIBRARY_DIRS 
    ${FFMPEG_ROOT}/lib
)

# 设置FFmpeg库文件
set(FFMPEG_LIBRARIES
    ${FFMPEG_LIBRARY_DIRS}/avformat.lib
    ${FFMPEG_LIBRARY_DIRS}/avcodec.lib
    ${FFMPEG_LIBRARY_DIRS}/avutil.lib
    ${FFMPEG_LIBRARY_DIRS}/swresample.lib
    ${FFMPEG_LIBRARY_DIRS}/swscale.lib
    ${FFMPEG_LIBRARY_DIRS}/avdevice.lib
    ${FFMPEG_LIBRARY_DIRS}/avfilter.lib
    ${FFMPEG_LIBRARY_DIRS}/postproc.lib
)

# 设置FFmpeg DLL目录
set(FFMPEG_DLL_DIRS 
    ${FFMPEG_ROOT}/bin
)

# 包含目录
include_directories(
    ${FFMPEG_INCLUDE_DIRS}
    ${Python_INCLUDE_DIRS}
)

# 链接目录
link_directories(
    ${FFMPEG_LIBRARY_DIRS}
)

# 创建核心库
add_library(avmerger_lib STATIC
    AudioVideoMerger.cpp
)

target_include_directories(avmerger_lib PUBLIC
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(avmerger_lib
    ${FFMPEG_LIBRARIES}
)

# 创建Python绑定
pybind11_add_module(avmerger pybind.cpp)

target_link_libraries(avmerger PRIVATE 
    avmerger_lib
)

# 复制FFmpeg DLL文件到输出目录（仅在Windows上）
if(WIN32)
    # 获取FFmpeg所需的DLL文件列表
    set(FFMPEG_DLLS
        "${FFMPEG_DLL_DIRS}/avcodec-61.dll"
        "${FFMPEG_DLL_DIRS}/avdevice-61.dll"
        "${FFMPEG_DLL_DIRS}/avfilter-10.dll"
        "${FFMPEG_DLL_DIRS}/avformat-61.dll"
        "${FFMPEG_DLL_DIRS}/avutil-59.dll"
        "${FFMPEG_DLL_DIRS}/postproc-58.dll"
        "${FFMPEG_DLL_DIRS}/swresample-5.dll"
        "${FFMPEG_DLL_DIRS}/swscale-8.dll"
    )
    
    # 创建自定义命令来复制DLL文件
    add_custom_command(TARGET avmerger POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${FFMPEG_DLLS}
        $<TARGET_FILE_DIR:avmerger>
    )
endif()

# 安装规则
install(TARGETS avmerger
    LIBRARY DESTINATION .
    RUNTIME DESTINATION .
)

# 如果需要复制DLL到安装目录
if(WIN32)
    install(FILES ${FFMPEG_DLLS}
        DESTINATION .
    )
endif()
