#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(author_api_test
    author_api_test.cpp
    )
target_link_libraries(author_api_test
    author_api_service
    blob
    hasher
    testutil_primitives_generator
    transaction_pool
    dummy_error
    )
