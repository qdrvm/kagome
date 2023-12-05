#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(block_builder
    impl/block_builder_impl.cpp
    impl/block_builder_error.cpp
    impl/proposer_impl.cpp
    impl/block_builder_factory_impl.cpp
    )
target_link_libraries(block_builder
    outcome
    logger
    blockchain
    scale::scale
    metrics
    )
kagome_clear_objects(block_builder)
