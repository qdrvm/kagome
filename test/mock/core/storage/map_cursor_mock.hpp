/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_STORAGE_MAP_CURSOR_MOCK
#define KAGOME_TEST_MOCK_CORE_STORAGE_MAP_CURSOR_MOCK

#include "storage/face/map_cursor.hpp"

#include <gmock/gmock.h>

namespace kagome::storage::face {

  template <typename K, typename V>
  class MapCursorMock : public MapCursor<K, V> {
   public:
    MOCK_METHOD(outcome::result<bool>, seekFirst, (), (override));

    MOCK_METHOD1_T(seek, outcome::result<bool>(const K &key));

    MOCK_METHOD(outcome::result<bool>, seekLast, (), (override));

    MOCK_METHOD(bool, isValid, (), (const, override));

    MOCK_METHOD(outcome::result<void>, next, (), (override));

    MOCK_METHOD(outcome::result<void>, prev, (), (override));

    MOCK_CONST_METHOD0_T(key, std::optional<K>());
    MOCK_CONST_METHOD0_T(value, std::optional<V>());
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_MAP_CURSOR_MOCK
