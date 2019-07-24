#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(block_header_repository
    impl/level_db_block_header_repository.cpp
    block_header_repository.cpp
    )
target_link_libraries(block_header_repository
    blockchain_common
    )
