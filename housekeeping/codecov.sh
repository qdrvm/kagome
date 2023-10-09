#!/bin/bash -xe
#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

buildDir=${BUILD_DIR:-${1:?BUILD_DIR variable or script arg is not defined}}
token=${CODECOV_TOKEN:-${2:?CODECOV_TOKEN variable or script arg is not defined}}

which git


if [ -z "$buildDir" ]; then
    echo "buildDir is empty"
    exit 1
fi

if [ -z "$token" ]; then
    echo "token arg is empty"
    exit 2
fi

bash <(curl -s https://codecov.io/bash) -s $buildDir -t $token
