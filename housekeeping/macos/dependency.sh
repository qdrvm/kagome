#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

# install python pip3 deps
python3.12 -m venv /venv
source /venv/bin/activate
python3.12 -m pip install cmake==3.25 requests gitpython gcovr scikit-build pyyml
curl https://sh.rustup.rs -sSf | sh -s -- -y --default-toolchain 1.75.0 --profile minimal

which python3
