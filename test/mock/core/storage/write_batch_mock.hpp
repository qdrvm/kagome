/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_WRITE_BATCH_MOCK_HPP
#define KAGOME_WRITE_BATCH_MOCK_HPP

#include "storage/face/write_batch.hpp"

#include <gmock/gmock.h>

namespace kagome::storage::face {

  template <typename K, typename V>
  class WriteBatchMock : public WriteBatch<K, V> {
   public:
    MOCK_METHOD(outcome::result<void>, commit, (), (override));

    MOCK_METHOD(void, clear, (), (override));

    MOCK_METHOD2_T(put, outcome::result<void>(const K &key, const V &value));
    outcome::result<void> put(const K &key, OwnedOrViewOf<V> &&value) override {
      return put(key, value.mut());
    }

    MOCK_METHOD1_T(remove, outcome::result<void>(const K &key));
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_WRITE_BATCH_MOCK_HPP
