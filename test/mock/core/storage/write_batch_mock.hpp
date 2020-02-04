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
    MOCK_METHOD0(commit, outcome::result<void> ());
    MOCK_METHOD0(clear, void ());
    MOCK_METHOD2_T(put, outcome::result<void>(const K &key, const V &value));
    outcome::result<void> put(const K &key, V &&value) {
      return put_rvalue(key, std::move(value));
    }
    MOCK_METHOD2_T(put_rvalue, outcome::result<void>(const K &key, V value));

    MOCK_METHOD1_T(remove, outcome::result<void> (const K &key));
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_WRITE_BATCH_MOCK_HPP
