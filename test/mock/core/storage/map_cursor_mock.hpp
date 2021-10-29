/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
    MOCK_METHOD0(seekFirst, outcome::result<bool>());

    MOCK_METHOD1_T(seek, outcome::result<bool>(const K &key));

    MOCK_METHOD0(seekLast, outcome::result<bool>());

    MOCK_CONST_METHOD0(isValid, bool());

    MOCK_METHOD0(next, outcome::result<void>());
    MOCK_METHOD0(prev, outcome::result<void>());

    MOCK_CONST_METHOD0_T(key, std::optional<K>());
    MOCK_CONST_METHOD0_T(value, std::optional<V>());
  };

}  // namespace kagome::storage::face

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_MAP_CURSOR_MOCK
