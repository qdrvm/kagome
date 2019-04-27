# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(leveldb_fs_test leveldb_fs_test.cpp)
target_link_libraries(leveldb_fs_test
    leveldb
    base_fs_test
    )

addtest(leveldb_integration_test
    leveldb_integration_test.cpp
    )
target_link_libraries(leveldb_integration_test
    leveldb
    base_leveldb_test
    Boost::filesystem
    )
