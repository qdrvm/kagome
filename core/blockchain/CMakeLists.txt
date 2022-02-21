#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(block_header_repository
    impl/block_header_repository_impl.cpp
    )
target_link_libraries(block_header_repository
    blockchain_common
    block_tree_error
    )
kagome_install(block_header_repository)

add_library(block_storage_error
    impl/block_storage_error.cpp
    )
target_link_libraries(block_storage_error
    outcome
    )

add_library(block_storage
    impl/block_storage_impl.cpp
    )
target_link_libraries(block_storage
    blockchain_common
    block_storage_error
    hasher
    scale::scale
    )

add_library(block_tree_error
    block_tree_error.hpp
    impl/block_tree_error.cpp
    )
target_link_libraries(block_tree_error
    outcome
    )
kagome_install(block_tree_error)
