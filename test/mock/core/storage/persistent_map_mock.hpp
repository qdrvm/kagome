/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_PERSISTENT_MAP_MOCK_HPP
#define KAGOME_PERSISTENT_MAP_MOCK_HPP

#include <gmock/gmock.h>

#include "storage/face/batchable.hpp"

namespace kagome::storage::face {
  template <typename K, typename V>
  struct GenericStorageMock : public face::GenericStorage<K, V> {
    MOCK_METHOD0_T(batch, std::unique_ptr<WriteBatch<K, V>>());

    MOCK_METHOD0_T(cursor, std::unique_ptr<MapCursor<K, V>>());

    MOCK_CONST_METHOD1_T(get, outcome::result<V>(const K &));

    MOCK_CONST_METHOD1_T(contains, bool(const K &));

    MOCK_METHOD2_T(put, outcome::result<void>(const K &, const V &));

    outcome::result<void> put(const K &k, V &&v) {
      return put_rvalueHack(k, std::move(v));
    }
    MOCK_METHOD2_T(put_rvalueHack, outcome::result<void>(const K &, V));

    MOCK_METHOD1_T(remove, outcome::result<void>(const K &));
  };
}  // namespace kagome::storage::face

#endif  // KAGOME_PERSISTENT_MAP_MOCK_HPP
