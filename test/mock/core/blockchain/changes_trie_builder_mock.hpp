/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_BLOCKCHAIN_CHANGES_TRIE_BUILDER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_BLOCKCHAIN_CHANGES_TRIE_BUILDER_MOCK_HPP

#include "blockchain/changes_trie_builder.hpp"

#include <gmock/gmock.h>

namespace kagome::blockchain {

  class ChangesTrieBuilderMock : public ChangesTrieBuilder {
   public:
    MOCK_METHOD1(
        startNewTrie,
        outcome::result<std::reference_wrapper<ChangesTrieBuilder>> 
        (primitives::BlockHash parent));

  MOCK_CONST_METHOD0(getConfig, outcome::result<OptChangesTrieConfig>());

    MOCK_METHOD2(insertExtrinsicsChange,
                 outcome::result<void>(
                     const common::Buffer &key,
                     const std::vector<primitives::ExtrinsicIndex> &changers));

    MOCK_METHOD0(finishAndGetHash, common::Hash256());
  };

}  // namespace kagome::blockchain

#endif  // KAGOME_TEST_MOCK_CORE_BLOCKCHAIN_CHANGES_TRIE_BUILDER_MOCK_HPP
