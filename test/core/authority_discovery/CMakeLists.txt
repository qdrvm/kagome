#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(address_publisher_test
    address_publisher_test.cpp
    )
target_link_libraries(address_publisher_test
    address_publisher
    key_store
    logger_for_tests
    network
    )

addtest(audi_query_test
    query.cpp
    )
target_link_libraries(audi_query_test
    address_publisher
    in_memory_storage
    key_store
    logger_for_tests
    )
