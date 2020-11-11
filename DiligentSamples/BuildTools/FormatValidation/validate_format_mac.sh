#!/bin/bash
python ../../../DiligentCore/BuildTools/FormatValidation/clang-format-validate.py \
--clang-format-executable ../../../DiligentCore/BuildTools/FormatValidation/clang-format_mac_10.0.0 \
-r ../../SampleBase ../../Tutorials ../../Samples \
--exclude ../../SampleBase/src/UWP \
--exclude ../../SampleBase/src/Win32/resources
