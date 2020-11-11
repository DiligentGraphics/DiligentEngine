#!/usr/bin/env bash

function reset_build {
    for DIR in "_build" "_installed"
    do
        if [ -d $DIR ]; then
            rm -rf $DIR
        fi
        mkdir -p $DIR
    done
}
function run_volk_test {
    for FILE in "./volk_test" "./volk_test.exe" "Debug/volk_test.exe" "Release/volk_test.exe"
    do
        if [ -f $FILE ]; then
            echo "Running test:"
            $FILE
            RC=$?
            break
        fi
    done
    echo "volk_test return code: $RC"
}

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
pushd $SCRIPT_DIR/..

reset_build
pushd _build
cmake -DCMAKE_INSTALL_PREFIX=../_installed -DVOLK_INSTALL=ON .. || exit 1
cmake --build . --target install || exit 1
popd

echo
echo "cmake_using_source_directly =======================================>"
echo 

pushd test/cmake_using_source_directly
reset_build
pushd _build
cmake .. || exit 1
cmake --build . || exit 1
run_volk_test
popd
popd

echo
echo "cmake_using_subdir_static =======================================>"
echo 

pushd test/cmake_using_subdir_static
reset_build
pushd _build
cmake .. || exit 1
cmake --build . || exit 1
run_volk_test
popd
popd

echo
echo "cmake_using_subdir_headers =======================================>"
echo 

pushd test/cmake_using_subdir_headers
reset_build
pushd _build
cmake .. || exit 1
cmake --build . || exit 1
run_volk_test
popd
popd

echo
echo "cmake_using_installed_headers =======================================>"
echo 

pushd test/cmake_using_installed_headers
reset_build
pushd _build
cmake -DCMAKE_INSTALL_PREFIX=../../../_installed/lib/cmake .. || exit 1
cmake --build . || exit 1
run_volk_test
popd
popd

popd

