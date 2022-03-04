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
    MOCK_METHOD(outcome::result<bool>, seekFirst, (), (override));

    MOCK_METHOD(outcome::result<bool>, seek, (const BufferView &), (override));

    MOCK_METHOD(outcome::result<void>,
                seekLowerBound,
                (const BufferView &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                seekUpperBound,
                (const BufferView &),
                (override));

    MOCK_METHOD(outcome::result<bool>, seekLast, (), (override));

    MOCK_METHOD(bool, isValid, (), (const, override));

    MOCK_METHOD(outcome::result<void>, next, (), (override));

    MOCK_METHOD(std::optional<common::Buffer>, key, (), (const, override));

    MOCK_METHOD(std::optional<common::BufferConstRef>,
                value,
                (),
                (const, override));
  };
}  // namespace kagome::storage::trie

#endif  // KAGOME_POLKADOT_TRIE_CURSOR_MOCK_H
