#!/usr/bin/env bash

set -ex

current_dir=$(dirname $(readlink -f "$0"))
parent_dir=$(dirname "$current_dir")

set -a; source $current_dir/.env; set +a #include .env vars

# install rust if not installed
if ! command -v rustc &> /dev/null
then
    curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION} && \
        rustup default ${RUST_VERSION}
fi

source $parent_dir/venv/bin/activate
$parent_dir/venv/bin/cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
$parent_dir/venv/bin/cmake --build build --target kagome -j ${CMAKE_CORE_NUMBER}
