#!/bin/bash

## Get the path of this file no matter where it is called from
## Solution from: https://stackoverflow.com/a/246128/2140449
VALIDATE_FORMAT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

errcho(){ echo "$@" 1>&2; }

function find_validator_bin() {
  local BIN=$(find "$VALIDATE_FORMAT_DIR" -name 'clang-format_linux_*')
  
  ## Try to launch the bin
  eval "$BIN --version >/dev/null 2> /dev/null"
  if [ $? -ne 0 ]; then
    ## BIN failed to run, try to get a system installed clang-format
    local SYS_BIN=$(which clang-format 2> /dev/null)
    if [ $? -ne 0 ]; then
      errcho "WARNING: skipping format validation as no suitable executable was found"
      BIN=""
    else
      local BIN_VERSION=$(echo $BIN | grep -Eo '[0-9]+\.[0-9]+\.[0-9]+')
      local SYS_BIN_VERSION=$(eval "$SYS_BIN --version" | grep -Eo '[0-9]+\.[0-9]+\.[0-9]+')
      if [ "$BIN_VERSION" != "$SYS_BIN_VERSION" ]; then
        errcho "WARNING: could not load the provided clang-format for validation."
        errcho "   clang-format exists in the system path however its version is $SYS_BIN_VERSION instead of $BIN_VERSION"
        errcho "   Should the validation fail, you can try skipping it by setting the cmake option:"
        errcho "   DILIGENT_SKIP_FORMAT_VALIDATION"
      fi
      BIN="$SYS_BIN"
    fi
  fi
  echo "$BIN"
}

function validate_format() {
  local BIN=$(find_validator_bin)
  if [ ! -z "$BIN" ]; then
    python "$VALIDATE_FORMAT_DIR/clang-format-validate.py" --clang-format-executable "$BIN" -r "$@"
  fi
}

## Example usage:
#
# #!/bin/bash
# source /PATH/TO/THIS/FILE/validate_format_linux_implementation.sh 
#
# validate_format ../../Common ../../Graphics ../../Platforms ../../Primitives ../../Tests \
#   --exclude ../../Graphics/HLSL2GLSLConverterLib/include/GLSLDefinitions.h \
#   --exclude ../../Graphics/HLSL2GLSLConverterLib/include/GLSLDefinitions_inc.h \
#   --exclude ../../Graphics/GraphicsEngineVulkan/shaders/GenerateMipsCS_inc.h \
#   --exclude ../../Tests/DiligentCoreAPITest/assets/*
#
