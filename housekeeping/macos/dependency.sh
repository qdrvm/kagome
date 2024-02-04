#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

if [[ "${KAGOME_MAC_CI}" = 1 ]]; then
  python3 -m venv ~/venv
  source ~/venv/bin/activate
fi

# install python pip3 deps
sudo python3 -m pip install --upgrade pip
sudo python3 -m pip install scikit-build
sudo python3 -m pip install cmake==3.25 requests gitpython gcovr

curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain 1.75.0 --profile minimal
brew install ninja
