#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(block_header_repository
    impl/key_value_block_header_repository.cpp
    impl/block_header_repository.cpp
    )
target_link_libraries(block_header_repository
    blockchain_common
    )

add_library(block_tree_error
    block_tree_error.hpp
    impl/block_tree_error.cpp
    )
target_link_libraries(block_tree_error
    outcome
    )
