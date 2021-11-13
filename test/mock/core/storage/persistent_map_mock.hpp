/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PERSISTENT_MAP_MOCK_HPP
#define KAGOME_PERSISTENT_MAP_MOCK_HPP

#include <gmock/gmock.h>

#include "storage/face/batchable.hpp"
#include "storage/face/generic_storage.hpp"

namespace kagome::storage::face {
  template <typename K, typename V>
  struct GenericStorageMock : public face::GenericStorage<K, V> {
    MOCK_METHOD0_T(batch, std::unique_ptr<WriteBatch<K, V>>());

    MOCK_METHOD0_T(cursor, std::unique_ptr<MapCursor<K, V>>());

    MOCK_CONST_METHOD1_T(get, outcome::result<V>(const K &));

    MOCK_CONST_METHOD1_T(tryGet, outcome::result<std::optional<V>>(const K &));

    MOCK_CONST_METHOD1_T(contains, outcome::result<bool>(const K &));

    MOCK_CONST_METHOD0_T(empty, bool());

    MOCK_METHOD(outcome::result<void>, put, (const K &, const V &), (override));
    outcome::result<void> put(const K &k, V &&v) override {
      return put(k, v);
    }

    MOCK_METHOD1_T(remove, outcome::result<void>(const K &));
  };
}  // namespace kagome::storage::face

#endif  // KAGOME_PERSISTENT_MAP_MOCK_HPP
