##
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
##

addtest(block_header_repository_test
    block_header_repository_test.cpp
    )
target_link_libraries(block_header_repository_test
    block_header_repository
    base_leveldb_test
    hasher
    )
