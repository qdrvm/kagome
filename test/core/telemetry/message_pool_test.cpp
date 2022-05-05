/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "telemetry/impl/message_pool.hpp"

#include <cstring>
#include <string>

#include <gtest/gtest.h>

using kagome::telemetry::MessageHandle;
using kagome::telemetry::MessagePool;

static constexpr auto kMaxRecordSizeBytes = 10;
static constexpr auto kMaxPoolCapacity = 3;

class MessagePoolTest : public ::testing::Test {
 public:
  using Prepared = std::pair<std::string, std::optional<MessageHandle>>;
  std::string testMessage(int length, char filler) {
    std::string message;
    message.resize(length, filler);
    return message;
  }

  Prepared composeAndPush(char filler) {
    auto message = testMessage(kMaxRecordSizeBytes, filler);
    auto handle = pool_.push(message, 1);
    return std::make_pair(message, handle);
  }

  bool verify(const Prepared &subject) {
    auto buffer = pool_[*subject.second];
    return not memcmp(
        subject.first.data(), buffer.data(), subject.first.length());
  }

 protected:
  MessagePool pool_{kMaxRecordSizeBytes, kMaxPoolCapacity};
};

/**
 * Message pool can be correctly initialized
 * @given a desired pool capacity
 * @when the pool is initialized
 * @then there are no exceptions and the capacity matches requested
 */
TEST_F(MessagePoolTest, CorrectInitialization) {
  ASSERT_EQ(kMaxPoolCapacity, pool_.capacity());
}

/**
 * @given a message with maximum allowed length
 * @when the message is passed to a message pool
 * @then it could be correctly served
 */
TEST_F(MessagePoolTest, StoreReadRelease) {
  auto message = testMessage(kMaxRecordSizeBytes, 'a');
  auto handle = pool_.push(message, 1);
  ASSERT_TRUE(handle);
  auto buffer = pool_[*handle];
  ASSERT_FALSE(memcmp(message.data(), buffer.data(), message.length()));
}

/**
 * @given a pool of known capacity
 * @when the maximum amount of messages is pushed to the pool
 * @then every message is stored and not corrupted
 */
TEST_F(MessagePoolTest, MaxLengthMessages) {
  std::vector<Prepared> prepared;
  for (int i = 0; i < kMaxPoolCapacity; ++i) {
    prepared.emplace_back(composeAndPush('a' + i));
  }
  for (const auto &p : prepared) {
    ASSERT_TRUE(p.second);
    ASSERT_TRUE(verify(p));
  }
}

/**
 * @given a pool full of messages
 * @when an extra message tried to be pushed to the pool
 * @then no handle to message is returned and other records are not corrupted
 */
TEST_F(MessagePoolTest, ExhaustCapacity) {
  std::vector<Prepared> prepared;
  for (int i = 0; i < kMaxPoolCapacity; ++i) {
    prepared.emplace_back(composeAndPush('a' + i));
  }
  auto extra_handle = pool_.push("extra", 1);
  for (const auto &p : prepared) {
    ASSERT_TRUE(p.second);
    ASSERT_TRUE(verify(p));
  }
  ASSERT_FALSE(extra_handle);
}

/**
 * @given a pool full of messages
 * @when a stored message is removed and another one tried to be inserted
 * @then all the previously stored messages and the new one are still stored and
 * not corrupted
 */
TEST_F(MessagePoolTest, MessageTrashing) {
  std::vector<Prepared> prepared;
  for (int i = 0; i < kMaxPoolCapacity; ++i) {
    prepared.emplace_back(composeAndPush('a' + i));
  }
  // verify initial push
  for (const auto &p : prepared) {
    ASSERT_TRUE(p.second);
    ASSERT_TRUE(verify(p));
  }
  pool_.release(*prepared[0].second);
  auto extra_handle = pool_.push("extra", 1);
  // place newly added record info instead of released item just for convenient
  // check
  prepared[0] = {"extra", extra_handle};
  // verify updated state
  // verify initial push
  for (const auto &p : prepared) {
    ASSERT_TRUE(p.second);
    ASSERT_TRUE(verify(p));
  }
}

/**
 * @given an initialized message pool
 * @when its capacity is requested
 * @then the proper value is reported
 */
TEST_F(MessagePoolTest, ReportedCapacity) {
  ASSERT_EQ(pool_.capacity(), kMaxPoolCapacity);
}

/**
 * @given a pool of size for a single entry only
 * @when entry's reference counter is more than one
 * @then proper amount of release calls should be performed to free the record
 */
TEST_F(MessagePoolTest, PartialRelease) {
  const auto kMaxEntries = 1;
  MessagePool pool(kMaxRecordSizeBytes, kMaxEntries);
  auto handle = pool.push("test", 2);  // initial refcount = 2
  ASSERT_TRUE(handle);
  pool.release(*handle);  // decrements refcount by 1

  auto next_handle = pool.push("test 2", 1);
  ASSERT_FALSE(next_handle);

  pool.release(*handle);  // completely releases the first message
  next_handle = pool.push("test 2", 1);
  ASSERT_TRUE(next_handle);
}

/**
 * @given a pool of size for a single entry only
 * @when there is a record with increased reference counter
 * @then proper amount of release calls should be performed to free the record
 */
TEST_F(MessagePoolTest, AddRefTest) {
  const auto kMaxEntries = 1;
  MessagePool pool(kMaxRecordSizeBytes, kMaxEntries);
  auto handle = pool.push("test", 1);  // initial refcount = 1
  ASSERT_TRUE(handle);
  pool.add_ref(*handle);  // refcount = 2

  auto next_handle =
      pool.push("test 2", 1);  // cannot push new record, no free slots
  ASSERT_FALSE(next_handle);
  pool.release(*handle);  // refcount = 1
  next_handle =
      pool.push("test 2", 1);  // still cannot push new record, no free slots
  ASSERT_FALSE(next_handle);
  pool.release(*handle);                 // refcount = 0, message released
  next_handle = pool.push("test 2", 1);  // message successfully pushed
  ASSERT_TRUE(next_handle);
}

/**
 * @given an empty message pool
 * @when bad handle is passed to operator[]
 * @then runtime execution gets failed
 */
TEST_F(MessagePoolTest, AccessBadHandle) {
  MessageHandle non_existing = 1;
  MessageHandle more_than_capacity = kMaxPoolCapacity + 1;
  auto access = [&](MessageHandle handle) { return pool_[handle]; };

  EXPECT_DEATH(access(non_existing), "handle_is_valid");
  EXPECT_DEATH(access(more_than_capacity), "handle_is_valid");
}

/**
 * @given an empty message pool
 * @when bad handle is passed to add_ref
 * @then runtime execution gets failed
 */
TEST_F(MessagePoolTest, AddRefBadHandle) {
  MessageHandle non_existing = 1;
  MessageHandle more_than_capacity = kMaxPoolCapacity + 1;
  auto add_ref = [&](MessageHandle handle) { return pool_.add_ref(handle); };

  EXPECT_DEATH(add_ref(non_existing), "handle_is_valid");
  EXPECT_DEATH(add_ref(more_than_capacity), "handle_is_valid");
}

/**
 * @given an empty message pool
 * @when bad handle is passed to release
 * @then runtime execution gets failed
 */
TEST_F(MessagePoolTest, ReleaseBadHandle) {
  MessageHandle non_existing = 1;
  MessageHandle more_than_capacity = kMaxPoolCapacity + 1;
  auto release = [&](MessageHandle handle) { return pool_.release(handle); };

  EXPECT_DEATH(release(non_existing), "handle_is_valid");
  EXPECT_DEATH(release(more_than_capacity), "handle_is_valid");
}

/**
 * @given a message pool
 * @when there is an attempt to push a record with non-valid reference counter
 * @then runtime execution gets failed
 */
TEST_F(MessagePoolTest, BadRefCount) {
  auto push = [&] {
    pool_.push("test", -1);
  };
  EXPECT_DEATH(push(), "ref_count >= 0");
}

/**
 * @given an attempt to initialize message pool of zero capacity
 * @when initialization takes a place
 * @then runtime execution would be failed in debug
 */
TEST(MessagePoolFreeTest, ZeroCapacity) {
  [[maybe_unused]] auto get_pool = [] {
    return MessagePool(kMaxRecordSizeBytes, 0);
  };
  EXPECT_DEBUG_DEATH(get_pool(), "entries_count > 0");
}

/**
 * @given an attempt to initialize message pool with zero messages size
 * @when initialization takes a place
 * @then runtime execution would be failed in debug
 */
TEST(MessagePoolFreeTest, ZeroSizeEntries) {
  [[maybe_unused]] auto get_pool = [] {
    return MessagePool(0, kMaxPoolCapacity);
  };
  EXPECT_DEBUG_DEATH(get_pool(), "entry_size_bytes > 0");
}
