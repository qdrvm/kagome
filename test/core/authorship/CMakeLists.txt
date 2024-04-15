#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(block_builder_test
    block_builder_test.cpp
    )

target_link_libraries(block_builder_test
    logger_for_tests
    )

addtest(block_builder_factory_test
    block_builder_factory_test.cpp
    )

target_link_libraries(block_builder_factory_test
    logger_for_tests
    )

addtest(proposer_test
    proposer_test.cpp
    )

target_link_libraries(proposer_test
    logger_for_tests
    )
