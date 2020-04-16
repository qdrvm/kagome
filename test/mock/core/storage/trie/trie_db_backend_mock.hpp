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

#ifndef KAGOME_TRIE_DB_BACKEND_MOCK_HPP
#define KAGOME_TRIE_DB_BACKEND_MOCK_HPP

#include <gmock/gmock.h>

#include "storage/trie/trie_db_backend.hpp"

namespace kagome::storage::trie {

  class TrieDbBackendMock: public TrieDbBackend {
   public:
    MOCK_METHOD0(batch, std::unique_ptr<face::WriteBatch<Buffer, Buffer>>());
    MOCK_METHOD0(cursor, std::unique_ptr<face::MapCursor<Buffer, Buffer>>());
    MOCK_CONST_METHOD1(get, outcome::result<Buffer>(const Buffer &key));
    MOCK_CONST_METHOD1(contains, bool (const Buffer &key));
    MOCK_METHOD2(put, outcome::result<void> (const Buffer &key, const Buffer &value));
    outcome::result<void> put(const common::Buffer &k, common::Buffer &&v) {
      return put_rvalueHack(k, std::move(v));
    }
    MOCK_METHOD2(put_rvalueHack,
                 outcome::result<void>(const common::Buffer &, common::Buffer));
    MOCK_METHOD1(remove, outcome::result<void> (const Buffer &key));
  };

}

#endif  // KAGOME_TRIE_DB_BACKEND_MOCK_HPP
