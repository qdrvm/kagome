#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(rocksdb_fs_test rocksdb_fs_test.cpp)
target_link_libraries(rocksdb_fs_test
    storage
    base_fs_test
    )

addtest(rocksdb_integration_test
    rocksdb_integration_test.cpp
    )
target_link_libraries(rocksdb_integration_test
    storage
    base_rocksdb_test
    filesystem
    hexutil
    )
