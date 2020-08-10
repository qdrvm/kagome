#!/bin/bash -xe

# install python pip3 deps
pip3 install --user pyyaml
sudo python3 -m pip install --upgrade pip
sudo python3 -m pip install scikit-build
sudo python3 -m pip install cmake requests gitpython gcovr
curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain nightly-2020-05-30 --profile minimal
export PATH="$HOME/.cargo/bin:$PATH"
