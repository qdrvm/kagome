#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

# install python pip3 deps
/opt/homebrew/opt/python@3.12/bin/python3.12 -m pip install --upgrade pip
/opt/homebrew/opt/python@3.12/bin/python3.12 -m pip install scikit-build
/opt/homebrew/opt/python@3.12/bin/python3.12 -m pip install cmake==3.25 requests gitpython gcovr
curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain 1.75.0 --profile minimal

which python3
