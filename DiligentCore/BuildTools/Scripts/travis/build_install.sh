mkdir build
cd build

if [ "$TRAVIS_OS_NAME" = "linux" ]; then 
  cmake .. -G "Unix Makefiles" $1 -DCMAKE_BUILD_TYPE=${CONFIG} -DCMAKE_INSTALL_PREFIX=install &&
  cmake --build . --target install
  # We must return now because otherwise the following if... command will reset the error code
  return
fi

if [ "$TRAVIS_OS_NAME" = "osx" ]; then 
  if [ "$IOS" = "true" ]; then 
    cmake .. $1 -DCMAKE_TOOLCHAIN_FILE=../DiligentCore/ios.toolchain.cmake -DIOS_PLATFORM=OS64 -DIOS_ARCH=arm64 -DVULKAN_SDK="$VULKAN_SDK" -DCMAKE_INSTALL_PREFIX=install -G "Xcode" || return
    XCODE_BUILD_SETTINGS="CODE_SIGN_IDENTITY= CODE_SIGNING_REQUIRED=NO"
  else
    cmake .. $1 -DCMAKE_INSTALL_PREFIX=install -G "Xcode" || return
    XCODE_BUILD_SETTINGS=""
  fi
  xcodebuild -target install -configuration ${CONFIG} ${XCODE_BUILD_SETTINGS} | xcpretty && return ${PIPESTATUS[0]}
fi
