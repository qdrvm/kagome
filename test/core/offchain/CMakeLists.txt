#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(uri_test
    uri_test.cpp
    )
target_link_libraries(uri_test
    http_request
    )

addtest(http_request_test
    http_request_test.cpp
    )
target_link_libraries(http_request_test
    logger_for_tests
    http_request
    )
