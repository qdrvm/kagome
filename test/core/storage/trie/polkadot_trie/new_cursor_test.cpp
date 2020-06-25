/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <random>

#include <gtest/gtest.h>

#include "storage/trie/polkadot_trie/polkadot_trie_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_node.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::storage::trie::PolkadotTrie;
using kagome::storage::trie::PolkadotTrieImpl;
using kagome::storage::trie::PolkadotNode;

class TrieIterator {
  TrieIterator(PolkadotTrie& trie, Buffer current_key):
  trie {trie} {
    current = trie.getNode(trie.getRoot(), current_key).value();
  }

  Buffer next() {
    // if has children, go to the lex lowest child
    //
  }

  PolkadotTrie& trie;
  std::weak_ptr<PolkadotNode> current;
};

std::shared_ptr<PolkadotTrie> makeTrie(
    const std::vector<std::pair<Buffer, Buffer>> &vals) {
  auto trie = std::make_shared<PolkadotTrieImpl>(
      [](auto b, uint8_t idx) { return b->children.at(idx); });
  for (auto &p : vals) {
    EXPECT_OUTCOME_TRUE_1(trie->put(p.first, p.second));
  }
  return trie;
}

TEST(TrieIterator, Works) {
  std::vector<std::pair<Buffer, Buffer>> vals{{"ab"_buf, Buffer{1}},
                                              {"ac"_buf, Buffer{3}},
                                              {"acd"_buf, Buffer{2}},
                                              {"e"_buf, Buffer{7}},
                                              {"f"_buf, Buffer{8}},
                                              {"fg"_buf, Buffer{4}},
                                              {"fh"_buf, Buffer{5}},
                                              {"fhi"_buf, Buffer{6}}};
  auto trie = makeTrie(vals);

}
