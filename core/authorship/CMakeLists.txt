#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
    block_header_repository
    scale::scale
    metrics
    )
add_custom_command(TARGET block_builder
    POST_BUILD
    COMMAND find ${CMAKE_CURRENT_BINARY_DIR} -name "*.o" -type f -delete
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Removing object files from '${CMAKE_CURRENT_BINARY_DIR}'"
)
