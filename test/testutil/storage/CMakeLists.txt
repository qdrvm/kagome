# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(base_fs_test
    base_fs_test.hpp
    base_fs_test.cpp
    )
target_link_libraries(base_fs_test
    Boost::filesystem
    Boost::boost
    logger_for_tests
    )

add_library(base_rocksdb_test
    base_rocksdb_test.hpp
    base_rocksdb_test.cpp
    )
target_link_libraries(base_rocksdb_test
    base_fs_test
    Boost::filesystem
    Boost::boost
    rocksdb_wrapper
    logger_for_tests
    )

add_library(std_list_adapter INTERFACE)

target_link_libraries(std_list_adapter INTERFACE
    outcome
    )
