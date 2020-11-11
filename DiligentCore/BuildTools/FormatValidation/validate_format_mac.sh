#!/bin/bash
python clang-format-validate.py --clang-format-executable ./clang-format_mac_10.0.0 \
-r ../../Common ../../Graphics ../../Platforms ../../Primitives ../../Tests \
--exclude ../../Graphics/HLSL2GLSLConverterLib/include/GLSLDefinitions.h \
--exclude ../../Graphics/HLSL2GLSLConverterLib/include/GLSLDefinitions_inc.h \
--exclude ../../Graphics/GraphicsEngineVulkan/shaders/GenerateMipsCS_inc.h \
--exclude ../../Tests/DiligentCoreAPITest/assets/*
