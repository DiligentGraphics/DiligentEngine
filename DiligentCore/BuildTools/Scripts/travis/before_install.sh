CMAKE_VERSION="3.18.2"
VULKAN_SDK_VER="1.2.135.0"

if [ "$TRAVIS_OS_NAME" = "osx" ];  then
  wget --no-check-certificate https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Darwin-x86_64.tar.gz &&
  tar -xzf cmake-${CMAKE_VERSION}-Darwin-x86_64.tar.gz
  export PATH=$PWD/cmake-${CMAKE_VERSION}-Darwin-x86_64/CMake.app/Contents/bin:$PATH
  cmake --version
  if [ "$IOS" = "true" ];  then
    wget -O vulkansdk-macos-$VULKAN_SDK_VER.tar.gz https://sdk.lunarg.com/sdk/download/$VULKAN_SDK_VER/mac/vulkansdk-macos-$VULKAN_SDK_VER.tar.gz?Human=true &&
    tar -xzf vulkansdk-macos-$VULKAN_SDK_VER.tar.gz
    export VULKAN_SDK=$PWD/vulkansdk-macos-$VULKAN_SDK_VER
  fi
fi

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  # Link gcc-9 and g++-9 to their standard commands
  sudo ln -s /usr/bin/gcc-9 /usr/local/bin/gcc
  sudo ln -s /usr/bin/g++-9 /usr/local/bin/g++
  # Export CC and CXX to tell cmake which compiler to use
  export CC=/usr/bin/gcc-9
  export CXX=/usr/bin/g++-9
  # Check versions of gcc, g++ and cmake
  gcc -v
  g++ -v
  # Download a recent cmake
  mkdir $HOME/usr
  export PATH="$HOME/usr/bin:$PATH"
  wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.sh &&
  chmod +x cmake-${CMAKE_VERSION}-Linux-x86_64.sh &&
  ./cmake-${CMAKE_VERSION}-Linux-x86_64.sh --prefix=$HOME/usr --exclude-subdir --skip-license
  cmake --version
  sudo apt-get update
  sudo apt-get install libx11-dev
  sudo apt-get install mesa-common-dev
  sudo apt-get install mesa-utils
  sudo apt-get install libgl-dev
fi

