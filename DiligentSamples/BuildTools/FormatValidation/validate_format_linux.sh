#!/bin/bash

source ../../../DiligentCore/BuildTools/FormatValidation/validate_format_linux_implementation.sh

validate_format ../../SampleBase ../../Tutorials ../../Samples \
--exclude ../../SampleBase/src/UWP \
--exclude ../../SampleBase/src/Win32/resources
