if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    $1/Tests/DiligentToolsTest/DiligentToolsTest || return
fi

if [ "$TRAVIS_OS_NAME" = "osx" ]; then 
  if [ "$IOS" = "false" ]; then
    $1/Tests/DiligentToolsTest/$CONFIG/DiligentToolsTest || return
  fi
fi
