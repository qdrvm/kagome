/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_POLKADOT_TRIE_CURSOR_MOCK_H
#define KAGOME_POLKADOT_TRIE_CURSOR_MOCK_H

#include "storage/trie/polkadot_trie/polkadot_trie_cursor_impl.hpp"

namespace kagome::storage::trie {

  class PolkadotTrieCursorMock : public PolkadotTrieCursor {
   public:
    MOCK_METHOD0(seekFirst, outcome::result<bool>());
    MOCK_METHOD1(seek, outcome::result<bool>(const Buffer &));
    MOCK_METHOD1(seekLowerBound, outcome::result<void>(const Buffer &));
    MOCK_METHOD1(seekUpperBound, outcome::result<void>(const Buffer &));
    MOCK_METHOD0(seekLast, outcome::result<bool>());
    MOCK_CONST_METHOD0(isValid, bool());
    MOCK_METHOD0(next, outcome::result<void>());
    MOCK_CONST_METHOD0(key, std::optional<common::Buffer>());
    MOCK_CONST_METHOD0(value, std::optional<common::Buffer>());
  };
}  // namespace kagome::storage::trie

#endif  // KAGOME_POLKADOT_TRIE_CURSOR_MOCK_H
