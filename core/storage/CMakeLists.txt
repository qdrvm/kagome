#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_library(storage
    map_prefix/prefix.cpp
    rocksdb/rocksdb_cursor.cpp
    rocksdb/rocksdb.cpp
    rocksdb/rocksdb_batch.cpp
    rocksdb/rocksdb_spaces.cpp
    database_error.cpp
    changes_trie/impl/storage_changes_tracker_impl.cpp
    in_memory/in_memory_storage.cpp
    trie/child_prefix.cpp
    trie/compact_decode.cpp
    trie/compact_encode.cpp
    trie/impl/trie_batch_base.cpp
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
    trie_pruner/impl/trie_pruner_impl.cpp
    )
target_link_libraries(storage
    blob
    scale::scale
    Boost::boost
    outcome
    RocksDB::rocksdb
    filesystem
    fmt::fmt
    logger
    blake2
    )
kagome_install(storage)
kagome_clear_objects(storage)
