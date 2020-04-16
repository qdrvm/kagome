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

#ifndef KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_DB_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_DB_MOCK_HPP

#include <gmock/gmock.h>

#include "storage/trie/trie_db.hpp"

namespace kagome::storage::trie {

  class TrieDbMock : public TrieDb {
   public:
    MOCK_METHOD0(
        batch,
        std::unique_ptr<face::WriteBatch<common::Buffer, common::Buffer>>());

    MOCK_METHOD0(
        cursor,
        std::unique_ptr<face::MapCursor<common::Buffer, common::Buffer>>());

    MOCK_CONST_METHOD1(get,
                       outcome::result<common::Buffer>(const common::Buffer &));

    MOCK_CONST_METHOD1(contains, bool(const common::Buffer &));

    MOCK_METHOD2(put,
                 outcome::result<void>(const common::Buffer &,
                                       const common::Buffer &));

    outcome::result<void> put(const common::Buffer &k, common::Buffer &&v) {
      return put_rvalueHack(k, std::move(v));
    }
    MOCK_METHOD2(put_rvalueHack,
                 outcome::result<void>(const common::Buffer &, common::Buffer));

    MOCK_METHOD1(remove, outcome::result<void>(const common::Buffer &));

    MOCK_CONST_METHOD0(getRootHash, common::Buffer());

    MOCK_METHOD1(clearPrefix, outcome::result<void>(const common::Buffer &buf));

    MOCK_CONST_METHOD0(empty, bool());
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TEST_MOCK_CORE_STORAGE_TRIE_TRIE_DB_MOCK_HPP
