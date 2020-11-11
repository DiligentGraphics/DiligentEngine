python clang-format-validate.py --color never --clang-format-executable clang-format_10.0.0.exe ^
-r ../../Common ../../Graphics ../../Platforms ../../Primitives ../../Tests ^
--exclude ../../Graphics/HLSL2GLSLConverterLib/include/GLSLDefinitions.h ^
--exclude ../../Graphics/HLSL2GLSLConverterLib/include/GLSLDefinitions_inc.h ^
--exclude ../../Graphics/GraphicsEngineVulkan/shaders/* ^
--exclude ../../Tests/DiligentCoreAPITest/assets/*
