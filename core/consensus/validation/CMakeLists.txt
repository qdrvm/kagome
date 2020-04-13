#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(block_validator
    babe_block_validator.cpp
    )
target_link_libraries(block_validator
    outcome
    babe_digests_util
    mp_utils
    logger
    )
