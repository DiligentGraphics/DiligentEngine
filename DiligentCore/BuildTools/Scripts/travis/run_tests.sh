if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    $1/Tests/DiligentCoreTest/DiligentCoreTest || return
fi

if [ "$TRAVIS_OS_NAME" = "osx" ]; then 
  if [ "$IOS" = "false" ]; then
    $1/Tests/DiligentCoreTest/$CONFIG/DiligentCoreTest || return
  fi
fi

