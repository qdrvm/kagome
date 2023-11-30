#!/bin/bash -xe

RUST_VERSION=nightly-2022-11-20
RUSTUP_HOME=~/.rustup
CARGO_HOME=~/.cargo
PATH="${CARGO_HOME}/bin:${PATH}"

./venv/bin/cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release
./venv/bin/cmake --build build --target kagome -j 6
