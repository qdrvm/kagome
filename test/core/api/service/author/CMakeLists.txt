#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(author_api_test
    author_api_test.cpp
    )
target_link_libraries(author_api_test
    api
    blob
    hasher
    testutil_primitives_generator
    transaction_pool
    dummy_error
    logger_for_tests
    )
