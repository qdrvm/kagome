/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PERSISTENT_MAP_MOCK_HPP
#define KAGOME_PERSISTENT_MAP_MOCK_HPP

#include <gmock/gmock.h>

#include "storage/buffer_map_types.hpp"
#include "storage/face/generic_maps.hpp"

namespace kagome::storage::face {
  template <typename K, typename V>
  struct GenericStorageMock : public GenericStorage<K, V> {
    MOCK_METHOD0_T(batch, std::unique_ptr<WriteBatch<K, V>>());

    MOCK_METHOD0_T(cursor, std::unique_ptr<MapCursor<K, V>>());

    MOCK_METHOD(outcome::result<V>, getMock, (const View<K> &), (const));

    MOCK_METHOD(outcome::result<std::optional<V>>,
                tryGetMock,
                (const View<K> &),
                (const));

    outcome::result<OwnedOrView<V>> get(const View<K> &key) const override {
      OUTCOME_TRY(value, getMock(key));
      return value;
    }

    outcome::result<std::optional<OwnedOrView<V>>> tryGet(
        const View<K> &key) const override {
      OUTCOME_TRY(value, tryGetMock(key));
      if (value) {
        return std::move(*value);
      }
      return std::nullopt;
    }

    MOCK_CONST_METHOD1_T(contains, outcome::result<bool>(const View<K> &));

    MOCK_CONST_METHOD0_T(empty, bool());

    MOCK_METHOD(outcome::result<void>, put, (const View<K> &, const V &));
    outcome::result<void> put(const View<K> &k, OwnedOrView<V> &&v) override {
      return put(k, v.mut());
    }

    MOCK_METHOD1_T(remove, outcome::result<void>(const View<K> &));

    MOCK_CONST_METHOD0_T(size, size_t());
  };
}  // namespace kagome::storage::face

namespace kagome::storage {
  using BufferStorageMock = face::GenericStorageMock<Buffer, Buffer>;
}  // namespace kagome::storage

#endif  // KAGOME_PERSISTENT_MAP_MOCK_HPP
