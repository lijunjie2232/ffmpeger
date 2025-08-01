cmake_minimum_required(VERSION 3.12)
project(avmerger)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# set anaconda python path
set(Python_ROOT_DIR "D:/sw/anaconda3/envs/ytdl")
set(Python_EXECUTABLE ${Python_ROOT_DIR}/python)
set(Python_INCLUDE_DIRS ${Python_ROOT_DIR}/include)
set(Python_LIBRARIES ${Python_ROOT_DIR}/libs)

# 查找Python
find_package(Python COMPONENTS Interpreter Development REQUIRED)

set(pybind11_DIR ${Python_ROOT_DIR}/Lib/site-packages/pybind11/share/cmake/pybind11)
# 查找pybind11
find_package(pybind11 REQUIRED)

# 设置FFmpeg路径 - 从环境变量或默认路径获取
if(DEFINED ENV{FFMPEG_ROOT})
    set(FFMPEG_ROOT $ENV{FFMPEG_ROOT})
else()
    set(FFMPEG_ROOT "C:/Program Files/ffmpeg")
endif()

message(STATUS "Using FFmpeg root: ${FFMPEG_ROOT}")

# 设置FFmpeg包含目录和库目录
set(FFMPEG_INCLUDE_DIRS ${FFMPEG_ROOT}/include)
set(FFMPEG_LIBRARY_DIRS ${FFMPEG_ROOT}/lib)
set(FFMPEG_DLL_DIRS ${FFMPEG_ROOT}/bin)

# 查找FFmpeg库
find_library(AVCODEC_LIBRARY
    NAMES avcodec
    PATHS ${FFMPEG_LIBRARY_DIRS}
    NO_DEFAULT_PATH
)

find_library(AVDEVICE_LIBRARY
    NAMES avdevice
    PATHS ${FFMPEG_LIBRARY_DIRS}
    NO_DEFAULT_PATH
)

find_library(AVFILTER_LIBRARY
    NAMES avfilter
    PATHS ${FFMPEG_LIBRARY_DIRS}
    NO_DEFAULT_PATH
)

find_library(AVFORMAT_LIBRARY
    NAMES avformat
    PATHS ${FFMPEG_LIBRARY_DIRS}
    NO_DEFAULT_PATH
)

find_library(AVUTIL_LIBRARY
    NAMES avutil
    PATHS ${FFMPEG_LIBRARY_DIRS}
    NO_DEFAULT_PATH
)

find_library(POSTPROC_LIBRARY
    NAMES postproc
    PATHS ${FFMPEG_LIBRARY_DIRS}
    NO_DEFAULT_PATH
)

find_library(SWRESAMPLE_LIBRARY
    NAMES swresample
    PATHS ${FFMPEG_LIBRARY_DIRS}
    NO_DEFAULT_PATH
)

find_library(SWSCALE_LIBRARY
    NAMES swscale
    PATHS ${FFMPEG_LIBRARY_DIRS}
    NO_DEFAULT_PATH
)

# 检查是否找到了所有必需的库
set(FFMPEG_LIBRARIES_FOUND TRUE)

if(NOT AVCODEC_LIBRARY)
    message(WARNING "Could not find avcodec library")
    set(FFMPEG_LIBRARIES_FOUND FALSE)
endif()

if(NOT AVDEVICE_LIBRARY)
    message(WARNING "Could not find avdevice library")
    set(FFMPEG_LIBRARIES_FOUND FALSE)
endif()

if(NOT AVFILTER_LIBRARY)
    message(WARNING "Could not find avfilter library")
    set(FFMPEG_LIBRARIES_FOUND FALSE)
endif()

if(NOT AVFORMAT_LIBRARY)
    message(WARNING "Could not find avformat library")
    set(FFMPEG_LIBRARIES_FOUND FALSE)
endif()

if(NOT AVUTIL_LIBRARY)
    message(WARNING "Could not find avutil library")
    set(FFMPEG_LIBRARIES_FOUND FALSE)
endif()

if(NOT POSTPROC_LIBRARY)
    message(WARNING "Could not find postproc library")
    set(FFMPEG_LIBRARIES_FOUND FALSE)
endif()

if(NOT SWRESAMPLE_LIBRARY)
    message(WARNING "Could not find swresample library")
    set(FFMPEG_LIBRARIES_FOUND FALSE)
endif()

if(NOT SWSCALE_LIBRARY)
    message(WARNING "Could not find swscale library")
    set(FFMPEG_LIBRARIES_FOUND FALSE)
endif()

if(NOT FFMPEG_LIBRARIES_FOUND)
    message(FATAL_ERROR "Could not find all required FFmpeg libraries")
endif()

# 设置FFmpeg库列表
set(FFMPEG_LIBRARIES
    ${AVCODEC_LIBRARY}
    ${AVDEVICE_LIBRARY}
    ${AVFILTER_LIBRARY}
    ${AVFORMAT_LIBRARY}
    ${AVUTIL_LIBRARY}
    ${POSTPROC_LIBRARY}
    ${SWRESAMPLE_LIBRARY}
    ${SWSCALE_LIBRARY}
)

# 包含目录
include_directories(${FFMPEG_INCLUDE_DIRS})

# 创建核心库
add_library(avmerger_core STATIC
    AudioVideoMerger.cpp
)

target_include_directories(avmerger_core PUBLIC
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(avmerger_core
    ${FFMPEG_LIBRARIES}
)

# 创建Python绑定模块
pybind11_add_module(avmerger pybind.cpp)

target_link_libraries(avmerger PRIVATE 
    avmerger_core
)

# Windows平台特殊处理
if(WIN32)
    # 定义需要的DLL文件
    set(REQUIRED_DLLS
        "avcodec-61.dll"
        "avdevice-61.dll"
        "avfilter-10.dll"
        "avformat-61.dll"
        "avutil-59.dll"
        "postproc-58.dll"
        "swresample-5.dll"
        "swscale-8.dll"
    )
    
    # 创建自定义命令来复制DLL文件到输出目录
    foreach(dll ${REQUIRED_DLLS})
        set(DLL_PATH "${FFMPEG_DLL_DIRS}/${dll}")
        if(EXISTS ${DLL_PATH})
            add_custom_command(TARGET avmerger POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DLL_PATH}
                $<TARGET_FILE_DIR:avmerger>
            )
            message(STATUS "Will copy ${dll} to output directory")
        else()
            message(WARNING "DLL not found: ${DLL_PATH}")
        endif()
    endforeach()
    
    # 创建安装规则
    install(TARGETS avmerger
        LIBRARY DESTINATION .
        RUNTIME DESTINATION .
    )
    
    # 安装DLL文件
    foreach(dll ${REQUIRED_DLLS})
        set(DLL_PATH "${FFMPEG_DLL_DIRS}/${dll}")
        if(EXISTS ${DLL_PATH})
            install(FILES ${DLL_PATH}
                DESTINATION .
            )
        endif()
    endforeach()
endif()

# 创建Python包结构
# 这部分需要额外的脚本来处理，因为CMake本身不直接处理Python包结构
# 我们创建一个自定义目标来准备wheel包
add_custom_target(prepare_wheel
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/wheel/avmerger
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:avmerger> ${CMAKE_BINARY_DIR}/wheel/avmerger/
    COMMENT "Preparing wheel directory structure"
)

# 添加说明信息
message(STATUS "===========================================")
message(STATUS "FFmpeg Merger CMake Configuration Complete")
message(STATUS "FFmpeg Root: ${FFMPEG_ROOT}")
message(STATUS "Include Dirs: ${FFMPEG_INCLUDE_DIRS}")
message(STATUS "Library Dirs: ${FFMPEG_LIBRARY_DIRS}")
message(STATUS "===========================================")
message(STATUS "To build:")
message(STATUS "  mkdir build && cd build")
message(STATUS "  cmake .. -DFFMPEG_ROOT=\"C:/your/ffmpeg/path\"")
message(STATUS "  cmake --build . --config Release")
message(STATUS "===========================================")