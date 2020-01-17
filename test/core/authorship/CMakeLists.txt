#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(block_builder_test
    block_builder_test.cpp
    )

target_link_libraries(block_builder_test
    block_builder
    blob
    )

addtest(block_builder_factory_test
    block_builder_factory_test.cpp
    )

target_link_libraries(block_builder_factory_test
    block_builder_factory
    )

addtest(proposer_test
    proposer_test.cpp
    )

target_link_libraries(proposer_test
    proposer
    )
