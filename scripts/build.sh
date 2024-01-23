#!/usr/bin/env bash
set -ex
current_dir=$(dirname $(readlink -f "$0"))
parent_dir=$(dirname "$current_dir")
cd $parent_dir

set -a; source $current_dir/.env; set +a #include .env vars 

source $parent_dir/venv/bin/activate
$parent_dir/venv/bin/cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release -DHUNTER_CONFIGURATION_TYPES=Release
$parent_dir/venv/bin/cmake --build build --target kagome -j ${CMAKE_CORE_NUMBER}
