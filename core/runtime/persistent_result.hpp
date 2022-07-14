/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_PERSISTENT_RESULT_HPP
#define KAGOME_CORE_RUNTIME_PERSISTENT_RESULT_HPP

namespace kagome::runtime {

  template <typename Result>
  struct PersistentResult {
    Result result;
    storage::trie::RootHash new_storage_root;
  };

  template <>
  struct PersistentResult<void> {
    storage::trie::RootHash new_storage_root;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_PERSISTENT_RESULT_HPP
