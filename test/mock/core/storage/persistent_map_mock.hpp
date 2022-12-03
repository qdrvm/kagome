/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PERSISTENT_MAP_MOCK_HPP
#define KAGOME_PERSISTENT_MAP_MOCK_HPP

#include <gmock/gmock.h>

#include "storage/face/batch_writeable.hpp"
#include "storage/face/generic_maps.hpp"

namespace kagome::storage::face {
  template <typename K, typename V, typename KView = K>
  struct GenericStorageMock : public face::GenericStorage<K, V, KView> {
    MOCK_METHOD0_T(batch, std::unique_ptr<WriteBatch<KView, V>>());

    MOCK_METHOD0_T(
        cursor,
        std::unique_ptr<typename face::GenericStorage<K, V, KView>::Cursor>());

    MOCK_METHOD(outcome::result<V>, getMock, (const KView &), (const));

    MOCK_METHOD(outcome::result<std::optional<V>>,
                tryGetMock,
                (const KView &),
                (const));

    outcome::result<OwnedOrView<V>> get(const KView &key) const override {
      OUTCOME_TRY(value, getMock(key));
      return std::move(value);
    }

    outcome::result<std::optional<OwnedOrView<V>>> tryGet(
        const KView &key) const override {
      OUTCOME_TRY(value, tryGetMock(key));
      if (value) {
        return std::move(*value);
      }
      return std::nullopt;
    }

    MOCK_CONST_METHOD1_T(contains, outcome::result<bool>(const KView &));

    MOCK_CONST_METHOD0_T(empty, bool());

    MOCK_METHOD(outcome::result<void>, put, (const KView &, const V &));
    outcome::result<void> put(const KView &k, OwnedOrView<V> &&v) override {
      return put(k, v.mut());
    }

    MOCK_METHOD1_T(remove, outcome::result<void>(const KView &));

    MOCK_CONST_METHOD0_T(size, size_t());
  };
}  // namespace kagome::storage::face

#endif  // KAGOME_PERSISTENT_MAP_MOCK_HPP
