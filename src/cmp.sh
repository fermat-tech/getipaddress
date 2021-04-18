#!/bin/bash

cmake_build_type=Release

build_dir="../build"

if [[ ! -d "${build_dir}" ]]
then
    mkdir -m 0700 "${build_dir}"
fi

cd "${build_dir}" &&

cmake \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_BUILD_TYPE="$cmake_build_type" \
    -G "Unix Makefiles" \
    ../src &&

cmake --build .
