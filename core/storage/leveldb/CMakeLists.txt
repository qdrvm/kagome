# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(leveldb
    leveldb.cpp
    leveldb_batch.cpp
    leveldb_error.cpp
    leveldb_cursor.cpp
    )
target_link_libraries(leveldb
    leveldb::leveldb
    buffer
    logger
    )
