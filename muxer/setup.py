import os
import sys
import shutil
from pathlib import Path
from setuptools import setup, Extension, find_packages
from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir
import pybind11

# 获取FFmpeg路径
ffmpeg_root = os.environ.get("FFMPEG_ROOT", "C:/Program Files/ffmpeg")

# FFmpeg包含目录
ffmpeg_include = os.path.join(ffmpeg_root, "include")

# FFmpeg库目录
ffmpeg_lib = os.path.join(ffmpeg_root, "lib")

# FFmpeg DLL目录（Windows）
ffmpeg_bin = os.path.join(ffmpeg_root, "bin")

ext_modules = [
    Pybind11Extension(
        "avmerger",
        ["pybind.cpp", "AudioVideoMerger.cpp"],
        include_dirs=[
            pybind11.get_include(),
            ffmpeg_include,
        ],
        libraries=[
            "avcodec",
            "avdevice",
            "avfilter",
            "avformat",
            "avutil",
            "postproc",
            "swresample",
            "swscale",
        ],
        library_dirs=[ffmpeg_lib],
        cxx_std=14,
    ),
]

# Windows平台特殊处理
if sys.platform == "win32":
    import shutil
    from pathlib import Path

    class CustomBuildExt(build_ext):
        def run(self):
            build_ext.run(self)

            # 复制DLL文件到构建目录
            build_lib = Path(self.build_lib)
            dll_files = [
                "avcodec-61.dll",
                "avdevice-61.dll",
                "avfilter-10.dll",
                "avformat-61.dll",
                "avutil-59.dll",
                "postproc-58.dll",
                "swresample-5.dll",
                "swscale-8.dll",
            ]

            for dll in dll_files:
                src = os.path.join(ffmpeg_bin, dll)
                if os.path.exists(src):
                    dst = build_lib / dll
                    shutil.copy2(src, dst)
                    print(f"Copied {dll} to {dst}")

else:
    CustomBuildExt = build_ext

setup(
    name="avmerger",
    version="0.1.0",
    author="Your Name",
    author_email="2533584225@qq.com",
    description="Audio Video Merger using FFmpeg",
    long_description=(
        open("README.md", "r", encoding="utf-8").read()
        if os.path.exists("README.md")
        else None
    ),
    long_description_content_type="text/markdown",
    url="https://github.com/yourusername/avmerger",
    packages=find_packages(),
    ext_modules=ext_modules,
    cmdclass={"build_ext": CustomBuildExt},
    zip_safe=False,
    python_requires=">=3.6",
    install_requires=[
        "pybind11>=2.6.0",
    ],
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
        "Programming Language :: Python :: 3.14",
    ],
    package_data={
        "avmerger": ["*.dll"],  # 确保DLL文件包含在包中
    },
    include_package_data=True,
)
