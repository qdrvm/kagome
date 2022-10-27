#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(memory_profiler
    memory_profiler.cpp
    )

target_link_libraries(memory_profiler
    )

add_executable(storage_explorer
    storage_explorer.cpp
    ${BACKWARD_ENABLE}
    )
target_link_libraries(storage_explorer
    application_injector
    )
add_backward(storage_explorer)

add_executable(kagome-db-editor
    kagome_db_editor.cpp
    )
target_link_libraries(kagome-db-editor
    Backward::Backward
    Boost::Boost.DI
    block_storage
    changes_tracker
    hasher
    rocksdb_wrapper
    polkadot_codec
    polkadot_trie_factory
    runtime_upgrade_tracker
    trie_serializer
    trie_storage
    trie_storage_backend
    )
kagome_install(kagome-db-editor)
