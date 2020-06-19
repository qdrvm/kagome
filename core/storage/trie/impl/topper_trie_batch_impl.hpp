/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_TRIE_IMPL_TOPPER_TRIE_BATCH_IMPL
#define KAGOME_CORE_STORAGE_TRIE_IMPL_TOPPER_TRIE_BATCH_IMPL

#include "storage/trie/impl/trie_batches.hpp"

namespace kagome::storage::trie {

  class TopperTrieBatchImpl: public TopperTrieBatch {
    TopperTrieBatchImpl(std::weak_ptr<TrieBatch> parent);

  };

}  // namespace kagome::

#endif  // KAGOME_CORE_STORAGE_TRIE_IMPL_TOPPER_TRIE_BATCH_IMPL
