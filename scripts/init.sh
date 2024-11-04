#!/usr/bin/env bash
set -ex
current_dir=$(dirname $(readlink -f "$0"))
parent_dir=$(dirname "$current_dir")
cd $parent_dir

set -a; source $current_dir/.env; set +a #include .env vars

# if not using nix, install the following packages
if [ -z "$FLAKE_INITIATED" ]; then
    apt update
    apt install --no-install-recommends -y \
        build-essential git gcc ca-certificates python-is-python3 python3-pip \
        python3-venv curl libgmp-dev libncurses6 libncurses6-dev libnsl-dev libseccomp-dev
fi

python3 -m venv "$parent_dir/venv"
echo "Python environment created successfully in $parent_dir/venv"

if [ -z "$FLAKE_INITIATED" ]; then
    $parent_dir/venv/bin/pip install --no-cache-dir cmake==${CMAKE_VERSION} gitpython requests
else
    $parent_dir/venv/bin/pip install --no-cache-dir gitpython requests
fi
