#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(rocksdb_dep_test
    rocksdb_dep_test.cpp
    )
target_link_libraries(rocksdb_dep_test
    RocksDB::rocksdb
    )
