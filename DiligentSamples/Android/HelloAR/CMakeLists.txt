# Copyright (C) 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##

# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Import the ARCore library.
add_library(arcore SHARED IMPORTED)
# ARCORE_LIBPATH and ARCORE_INCLUDE are given by gradle to cmake build
set_target_properties(arcore PROPERTIES
    IMPORTED_LOCATION ${ARCORE_LIBPATH}/${ANDROID_ABI}/libarcore_sdk_c.so
    INTERFACE_INCLUDE_DIRECTORIES ${ARCORE_INCLUDE}
)

# This is the main app library.
add_library(HelloAR SHARED
           src/main/cpp/BackgroundRenderer.cc
           src/main/cpp/hello_ar_application.cc
           src/main/cpp/jni_interface.cc
           src/main/cpp/ObjectRenderer.cc
           src/main/cpp/PlaneRenderer.cc
           src/main/cpp/PointCloudRenderer.cc
           src/main/cpp/util.cc
           ../../Tutorials/Common/src/TexturedCube.cpp
           )

target_include_directories(HelloAR PRIVATE
           src/main/cpp
           )

target_link_libraries(HelloAR
                      android
                      log
                      GLESv3
                      arcore
                      Diligent-BuildSettings
                      Diligent-Common
                      Diligent-GraphicsEngineOpenGL-shared
                      Diligent-GraphicsTools
                      Diligent-TextureLoader)
