#!/bin/bash -xe

RUST_VERSION=nightly-2022-11-20
RUSTUP_HOME=~/.rustup
CARGO_HOME=~/.cargo
PATH="${CARGO_HOME}/bin:${PATH}"

current_dir=$(dirname "$0")
parent_dir=$(dirname "$current_dir")

$parent_dir/venv/bin/cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
$parent_dir/venv/bin/cmake --build build --target kagome -j 6
