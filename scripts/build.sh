#!/usr/bin/env bash
set -ex
current_dir=$(dirname $(readlink -f "$0"))
parent_dir=$(dirname "$current_dir")
cd $parent_dir

set -a; source $current_dir/.env; set +a #include .env vars 

source $parent_dir/venv/bin/activate
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
sed -i 's/lcurses/lncurses/' build/node/CMakeFiles/kagome.dir/link.txt
cmake --build build --target kagome -j ${CMAKE_CORE_NUMBER}
