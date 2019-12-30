#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(block_validator
    block_validator.hpp
    babe_block_validator.cpp
    babe_block_validator.hpp
    )
target_link_libraries(block_validator
    outcome
    binaryen_tagged_transaction_queue_api
    block_tree
    )
