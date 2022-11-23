# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

add_library(storage
    rocksdb/rocksdb_cursor.cpp
    rocksdb/rocksdb.cpp
    rocksdb/rocksdb_batch.cpp
    database_error.cpp
    changes_trie/impl/storage_changes_tracker_impl.cpp
    in_memory/in_memory_storage.cpp
    trie/impl/ephemeral_trie_batch_impl.cpp
    trie/impl/trie_storage_impl.cpp
    trie/impl/trie_storage_backend_batch.cpp
    trie/impl/trie_storage_backend_impl.cpp
    trie/impl/persistent_trie_batch_impl.cpp
    trie/impl/topper_trie_batch_impl.cpp
    trie/polkadot_trie/trie_node.cpp
    trie/polkadot_trie/polkadot_trie_impl.cpp
    trie/polkadot_trie/polkadot_trie_factory_impl.cpp
    trie/polkadot_trie/polkadot_trie_cursor_impl.cpp
    trie/polkadot_trie/trie_error.cpp
    trie/serialization/trie_serializer_impl.cpp
    trie/serialization/polkadot_codec.cpp
    )
target_link_libraries(storage
    blob
    scale::scale
    Boost::boost
    outcome
    RocksDB::rocksdb
    Boost::filesystem
    fmt::fmt
    logger
    )
add_custom_command(TARGET storage
    POST_BUILD
    COMMAND find ${CMAKE_CURRENT_BINARY_DIR} -name "*.o" -type f -delete
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Removing object files..."
)

kagome_install(storage)
