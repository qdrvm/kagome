# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0

addtest(changes_tracker_test
    changes_tracker_test.cpp
    )
target_link_libraries(changes_tracker_test
    changes_tracker
    trie_storage
    trie_serializer
    polkadot_trie_factory
    trie_storage_backend
    in_memory_storage
    polkadot_codec
    logger_for_tests
    )

