#!/bin/bash
python ../../../DiligentCore/BuildTools/FormatValidation/clang-format-validate.py \
--clang-format-executable ../../../DiligentCore/BuildTools/FormatValidation/clang-format_mac_10.0.0 \
-r ../../AssetLoader ../../Imgui ../../NativeApp/include  ../../NativeApp/src ../../TextureLoader ../../Tests \
--exclude ../../Imgui/interface/ImGuiImplMacOS.h \
--exclude ../../Imgui/interface/ImGuiImplIOS.h \
--exclude ../../NativeApp/src/UWP \
--exclude ../../NativeApp/include/UWP
