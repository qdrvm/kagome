/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_STORAGE_CHANGES_TRIE_IMPL_CHANGES_TRIE
#define KAGOME_CORE_STORAGE_CHANGES_TRIE_IMPL_CHANGES_TRIE

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "primitives/common.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::storage::changes_trie {

  class ChangesTrie {
   public:
    using ExtrinsicChanges =
        std::map<const common::Buffer &,
                 const std::vector<primitives::ExtrinsicIndex> &>;
    using BlocksChanges =
        std::map<const common::Buffer &,
                 const std::vector<primitives::BlockNumber> &>;

    outcome::result<common::Hash256> build() const {

    }

   private:


  };

}  // namespace kagome::storage::changes_trie

#endif  // KAGOME_CORE_STORAGE_CHANGES_TRIE_IMPL_CHANGES_TRIE
