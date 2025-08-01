#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "AudioVideoMerger.h"

namespace py = pybind11;

PYBIND11_MODULE(avmerger, m) {
    m.doc() = "Audio Video Merger module using FFmpeg";
    
    py::class_<AudioVideoMerger>(m, "AudioVideoMerger")
        .def(py::init<>())
        .def("merge", &AudioVideoMerger::merge, 
             "Merge audio and video files",
             py::arg("video_path"), 
             py::arg("audio_path"), 
             py::arg("output_path"))
        .def("get_last_error", &AudioVideoMerger::getLastError, 
             "Get last error message");
}
