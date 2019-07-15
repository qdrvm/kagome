##
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
##

add_library(block_header_repository
    impl/level_db_block_header_repository.cpp
    impl/level_db_util.cpp
    )
target_link_libraries(block_header_repository
    Boost::boost
    buffer
    primitives
    blob
    )
