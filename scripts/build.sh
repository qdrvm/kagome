#!/usr/bin/env bash

set -a; source $current_dir/.env; set +a #include .env vars 

source $parent_dir/venv/bin/activate
$parent_dir/venv/bin/cmake -H. -Bbuild -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCMAKE_BUILD_TYPE=Release
$parent_dir/venv/bin/cmake --build build --target kagome -j ${CMAKE_CORE_NUMBER}
