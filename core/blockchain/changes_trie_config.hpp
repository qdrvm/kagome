/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCKCHAIN_CHANGES_TRIE_CONFIG_HPP
#define KAGOME_BLOCKCHAIN_CHANGES_TRIE_CONFIG_HPP

namespace kagome::blockchain {

  struct ChangesTrieConfig {
    uint32_t digest_interval;
    uint32_t digest_levels;
  };

}

#endif  // KAGOME_BLOCKCHAIN_CHANGES_TRIE_CONFIG_HPP
