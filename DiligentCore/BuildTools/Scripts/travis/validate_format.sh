# The script must be run from FormatValidation folder

if [ "$TRAVIS_OS_NAME" = "osx" ];  then
  . ./validate_format_mac.sh
fi

if [ "$TRAVIS_OS_NAME" = "linux" ]; then
  . ./validate_format_linux.sh
fi

