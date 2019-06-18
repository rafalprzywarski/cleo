#!/bin/bash
set -e
MODE=DEBUG
BUILD_DIR=Debug
GTEST_FILTER=*
if [ "$1" == "--local" ]; then
    source local_build_and_test
fi
cmake -E make_directory ${BUILD_DIR}
cmake -E chdir ${BUILD_DIR} cmake .. -DCMAKE_BUILD_TYPE=${MODE}
cmake --build ${BUILD_DIR} -- -j3
${BUILD_DIR}/cleo_test "--gtest_filter=${GTEST_FILTER}"
(cd ${BUILD_DIR}; make install DESTDIR=../local_inst)
./local_inst/usr/local/bin/cleo test cleo.core.test
