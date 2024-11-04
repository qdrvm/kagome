#!/usr/bin/env bash
set -ex
current_dir=$(dirname $(readlink -f "$0"))
parent_dir=$(dirname "$current_dir")
cd $parent_dir

 #include .env vars and optionally .rust_env vars
set -a;
if [ -z "$FLAKE_INITIATED" ]; then
    source $current_dir/.rust_env;
fi
source $current_dir/.env;
set +a

source $parent_dir/venv/bin/activate
cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
sed -i 's/lcurses/lncurses/' build/node/CMakeFiles/kagome.dir/link.txt
cmake --build build --target kagome -j ${CMAKE_CORE_NUMBER}
