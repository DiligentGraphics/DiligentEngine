python ../../../DiligentCore/BuildTools/FormatValidation/clang-format-validate.py --color never ^
--clang-format-executable ../../../DiligentCore/BuildTools/FormatValidation/clang-format_10.0.0.exe ^
-r ../../AssetLoader ../../Imgui ../../NativeApp/include  ../../NativeApp/src ../../TextureLoader ../../Tests ^
--exclude ../../Imgui/interface/ImGuiImplMacOS.h ^
--exclude ../../Imgui/interface/ImGuiImplIOS.h ^
--exclude ../../NativeApp/src/UWP ^
--exclude ../../NativeApp/include/UWP
