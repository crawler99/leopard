#!/bin/sh

BUILD_DIR="build"
mkdir -p ${BUILD_DIR}

cd ${BUILD_DIR}
rm -fr debug
mkdir debug
cd debug

cmake                                  \
  -DCMAKE_BUILD_TYPE=Debug             \
  -DGTEST_ROOT=${RS_BUILDKIT}/gtest    \
  -DGMOCK_ROOT=${RS_BUILDKIT}/gmock    \
  -DGCC_ROOT=${RS_BUILDKIT}/gcc        \
  -DBUILD_TESTS=ON                     \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON   \
  ../..
