/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_PRUNER_MOCK_HPP
#define KAGOME_TRIE_PRUNER_MOCK_HPP

#include "storage/trie_pruner/trie_pruner.hpp"

#include "gmock/gmock.h"

namespace kagome::storage::trie_pruner {

  class TriePrunerMock : public TriePruner {
   public:
    MOCK_METHOD(outcome::result<void>,
                addNewState,
                (trie::RootHash const &state),
                (override));

    MOCK_METHOD(outcome::result<void>,
                prune,
                (trie::RootHash const &state),
                (override));
  };

}  // namespace kagome::storage::trie_pruner

#endif  // KAGOME_TRIE_PRUNER_MOCK_HPP
