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

#include "storage/trie/impl/trie_db_backend_impl.hpp"

#include <memory>

#include <gtest/gtest.h>
#include "mock/core/storage/persistent_map_mock.hpp"
#include "mock/core/storage/write_batch_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::storage::face::GenericStorageMock;
using kagome::storage::face::WriteBatchMock;
using kagome::storage::trie::TrieDbBackendImpl;
using testing::Invoke;
using testing::Return;

static const Buffer kNodePrefix{1};

class TrieDbBackendTest : public testing::Test {
 public:
  std::shared_ptr<GenericStorageMock<Buffer, Buffer>> storage =
      std::make_shared<GenericStorageMock<Buffer, Buffer>>();
  TrieDbBackendImpl backend{storage, kNodePrefix};
};

/**
 * @given trie backend
 * @when put a value to it
 * @then it puts a prefixed value to the storage
 */
TEST_F(TrieDbBackendTest, Put) {
  Buffer prefixed{kNodePrefix};
  prefixed.put("abc"_buf);
  EXPECT_CALL(*storage, put_rvalueHack(prefixed, "123"_buf))
      .WillOnce(Return(outcome::success()));
  EXPECT_OUTCOME_TRUE_1(backend.put("abc"_buf, "123"_buf));
}

/**
 * @given trie backend
 * @when get a value from it
 * @then it takes a prefixed value from the storage
 */
TEST_F(TrieDbBackendTest, Get) {
  Buffer prefixed{kNodePrefix};
  prefixed.put("abc"_buf);
  EXPECT_CALL(*storage, get(prefixed)).WillOnce(Return("123"_buf));
  EXPECT_OUTCOME_TRUE_1(backend.get("abc"_buf));
}

/**
 * @given trie backend batch
 * @when perform operations on it
 * @then it delegates them to the underlying storage batch with added prefixes
 */
TEST_F(TrieDbBackendTest, Batch) {
  auto batch_mock = std::make_unique<WriteBatchMock<Buffer, Buffer>>();
  EXPECT_CALL(*batch_mock,
              put_rvalue(Buffer{kNodePrefix}.put("abc"_buf), "123"_buf))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*batch_mock,
              put_rvalue(Buffer{kNodePrefix}.put("def"_buf), "123"_buf))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*batch_mock, remove(Buffer{kNodePrefix}.put("abc"_buf)))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*batch_mock, commit()).WillOnce(Return(outcome::success()));

  EXPECT_CALL(*storage, batch())
      .WillOnce(Return(testing::ByMove(std::move(batch_mock))));

  auto batch = backend.batch();
  EXPECT_OUTCOME_TRUE_1(batch->put("abc"_buf, "123"_buf));
  EXPECT_OUTCOME_TRUE_1(batch->put("def"_buf, "123"_buf));
  EXPECT_OUTCOME_TRUE_1(batch->remove("abc"_buf));
  EXPECT_OUTCOME_TRUE_1(batch->commit());
}
