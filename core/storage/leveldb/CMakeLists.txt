# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(leveldb_error
    leveldb_error.cpp
    )
target_link_libraries(leveldb_error
    outcome
    )

add_library(leveldb
    leveldb.cpp
    leveldb_batch.cpp
    leveldb_cursor.cpp
    )
target_link_libraries(leveldb
    leveldb::leveldb
    buffer
    leveldb_error
    logger
    )
kagome_install(leveldb)
kagome_install(leveldb_error)
