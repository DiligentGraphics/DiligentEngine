#!/bin/bash

source validate_format_linux_implementation.sh

validate_format ../../Common ../../Graphics ../../Platforms ../../Primitives ../../Tests \
  --exclude ../../Graphics/HLSL2GLSLConverterLib/include/GLSLDefinitions.h \
  --exclude ../../Graphics/HLSL2GLSLConverterLib/include/GLSLDefinitions_inc.h \
  --exclude ../../Graphics/GraphicsEngineVulkan/shaders/GenerateMipsCS_inc.h \
  --exclude ../../Tests/DiligentCoreAPITest/assets/*
