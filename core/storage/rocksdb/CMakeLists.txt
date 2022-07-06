#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(rocksdb_wrapper
    rocksdb.cpp
    rocksdb_batch.cpp
    rocksdb_cursor.cpp
    )
target_link_libraries(rocksdb_wrapper
    RocksDB::rocksdb
    Boost::filesystem
    buffer
    database_error
    logger
    )

kagome_install(rocksdb_wrapper)
