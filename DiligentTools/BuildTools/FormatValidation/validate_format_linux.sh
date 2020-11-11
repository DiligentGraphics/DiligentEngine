#!/bin/bash

source ../../../DiligentCore/BuildTools/FormatValidation/validate_format_linux_implementation.sh

validate_format ../../AssetLoader ../../Imgui ../../NativeApp/include  ../../NativeApp/src ../../TextureLoader ../../Tests \
--exclude ../../Imgui/interface/ImGuiImplMacOS.h \
--exclude ../../Imgui/interface/ImGuiImplIOS.h \
--exclude ../../NativeApp/src/UWP \
--exclude ../../NativeApp/include/UWP
