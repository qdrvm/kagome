#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(crypto_extension_test
    crypto_extension_test.cpp
    )
target_link_libraries(crypto_extension_test
    crypto_extension
    logger_for_tests
    )

addtest(io_extension_test
    io_extension_test.cpp
    )
target_link_libraries(io_extension_test
    io_extension
    logger_for_tests
    )

addtest(memory_extension_test
    memory_extension_test.cpp)
target_link_libraries(memory_extension_test
    memory_extension
    logger_for_tests
    )

addtest(misc_extension_test
    misc_extension_test.cpp
    )
target_link_libraries(misc_extension_test
    misc_extension
    executor
    logger_for_tests
    )

addtest(storage_extension_test
    storage_extension_test.cpp
    )
target_link_libraries(storage_extension_test
    storage_extension
    dummy_error
    logger_for_tests
    )

addtest(child_storage_extension_test
    child_storage_extension_test.cpp
    )
target_link_libraries(child_storage_extension_test
    child_storage_extension
    dummy_error
    logger_for_tests
    )

addtest(offchain_extension_test
    offchain_extension_test.cpp
    )
target_link_libraries(offchain_extension_test
    Boost::boost
    scale::scale_encode_append
    offchain_extension
    dummy_error
    logger_for_tests
    )
