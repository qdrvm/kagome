#!/bin/bash -xe

apt update
apt install --no-install-recommends -y build-essential git gcc ca-certificates python-is-python3 python3-pip python3-venv curl rustc

current_dir=$(dirname "$0")
parent_dir=$(dirname "$current_dir")
python3 -m venv "$parent_dir/venv"

echo "Python environment created successfully in $parent_dir/venv"

python -m venv venv
$parent_dir/venv/bin/pip install --no-cache-dir cmake==3.25 gitpython requests


RUST_VERSION=nightly-2022-11-20
RUSTUP_HOME=~/.rustup
CARGO_HOME=~/.cargo
PATH="${CARGO_HOME}/bin:${PATH}"
curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain ${RUST_VERSION} && \
    rustup default ${RUST_VERSION}
