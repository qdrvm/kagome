#!/usr/bin/env bash

set -a; source $current_dir/.env; set +a #include .env vars 

source $parent_dir/venv/bin/activate
$parent_dir/venv/bin/cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
$parent_dir/venv/bin/cmake --build build --target kagome -j ${CMAKE_CORE_NUMBER}
