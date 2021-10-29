#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(block_builder
    block_builder.hpp
    impl/block_builder_impl.cpp
    impl/block_builder_impl.hpp
    impl/block_builder_error.cpp
    impl/block_builder_error.hpp
    )
target_link_libraries(block_builder
    outcome
    buffer
    logger
    scale::scale
    )

add_library(block_builder_factory
    block_builder_factory.hpp
    impl/block_builder_factory_impl.cpp
    impl/block_builder_factory_impl.hpp
    )
target_link_libraries(block_builder_factory
    block_builder
    block_header_repository
    )

add_library(proposer
    proposer.hpp
    impl/proposer_impl.cpp
    impl/proposer_impl.hpp
    )
target_link_libraries(proposer
    block_builder_factory
    scale::scale
    )
