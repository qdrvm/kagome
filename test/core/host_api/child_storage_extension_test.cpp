/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/child_storage_extension.hpp"

#include <gtest/gtest.h>
#include <optional>
#include <scale/scale.hpp>

#include "common/monadic_utils.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/trie/polkadot_trie_cursor_mock.h"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "runtime/ptr_size.hpp"
#include "scale/encode_append.hpp"
#include "scale/kagome_scale.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "storage/trie/types.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/outcome/dummy_error.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/scale_test_comparator.hpp"
#include "testutil/runtime/memory.hpp"

using kagome::common::Buffer;
using kagome::common::BufferView;
using kagome::host_api::ChildStorageExtension;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::PtrSize;
using kagome::runtime::TestMemory;
using kagome::runtime::TrieStorageProviderMock;
using kagome::runtime::WasmOffset;
using kagome::runtime::WasmPointer;
using kagome::runtime::WasmSize;
using kagome::runtime::WasmSpan;
using kagome::storage::trie::PolkadotTrieCursorMock;
using kagome::storage::trie::RootHash;
using kagome::storage::trie::TrieBatchMock;
using kagome::storage::trie::TrieError;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

class ChildStorageExtensionTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    trie_child_storage_batch_ = std::make_shared<TrieBatchMock>();
    trie_batch_ = std::make_shared<TrieBatchMock>();
    storage_provider_ = std::make_shared<TrieStorageProviderMock>();
    EXPECT_CALL(*storage_provider_, getCurrentBatch())
        .WillRepeatedly(Return(trie_batch_));
    EXPECT_CALL(*storage_provider_, getChildBatchAt(_))
        .WillRepeatedly(
            Return(outcome::success(std::cref(*trie_child_storage_batch_))));
    EXPECT_CALL(*storage_provider_, getMutableChildBatchAt(_))
        .WillRepeatedly(
            Return(outcome::success(std::ref(*trie_child_storage_batch_))));
    memory_provider_ = std::make_shared<MemoryProviderMock>();
    EXPECT_CALL(*memory_provider_, getCurrentMemory())
        .WillRepeatedly(Return(std::ref(memory_)));
    child_storage_extension_ = std::make_shared<ChildStorageExtension>(
        storage_provider_, memory_provider_);
  }

 protected:
  std::shared_ptr<TrieBatchMock> trie_child_storage_batch_;
  std::shared_ptr<TrieBatchMock> trie_batch_;
  std::shared_ptr<TrieStorageProviderMock> storage_provider_;
  TestMemory memory_;
  std::shared_ptr<MemoryProviderMock> memory_provider_;
  std::shared_ptr<ChildStorageExtension> child_storage_extension_;
};

class VoidOutcomeParameterizedTest
    : public ChildStorageExtensionTest,
      public ::testing::WithParamInterface<outcome::result<void>> {};

class ReadOutcomeParameterizedTest
    : public ChildStorageExtensionTest,
      public ::testing::WithParamInterface<
          outcome::result<std::optional<Buffer>>> {};

class BoolOutcomeParameterizedTest
    : public ChildStorageExtensionTest,
      public ::testing::WithParamInterface<outcome::result<bool>> {};

/**
 * @given child storage key and key
 * @when invoke ext_default_child_storage_get_version_1
 * @then an expected value is fetched upon success or outcome::failure otherwise
 */
TEST_P(ReadOutcomeParameterizedTest, GetTest) {
  // executeOnChildStorage

  Buffer child_storage_key(8, 'l');
  Buffer key(8, 'k');

  // logic

  std::vector<uint8_t> encoded_opt_value;
  if (GetParam()) {
    encoded_opt_value =
        testutil::scaleEncodeAndCompareWithRef(GetParam().value()).value();
  }

  // 'func' (lambda)
  auto &param = GetParam();

  EXPECT_CALL(*trie_child_storage_batch_, tryGetMock(key.view()))
      .WillOnce(Return(param));

  // results
  auto call = [&] {
    return child_storage_extension_->ext_default_child_storage_get_version_1(
        memory_[child_storage_key], memory_[key]);
  };
  if (GetParam().has_error()) {
    ASSERT_THROW({ call(); }, std::runtime_error);
  } else {
    ASSERT_EQ(memory_[call()], encoded_opt_value);
  }
}

/**
 * @given child storage key, key, output buffer, offset
 * @when invoke ext_default_child_storage_read_version_1
 * @then upon success: read value from child storage by key, skip `offsset`
 * bytes, write rest of it to output buffer and return number of bytes written
 * upon failure: either TrieError::NO_VALUE or outcome::failure depending on
 * reason
 */
TEST_P(ReadOutcomeParameterizedTest, ReadTest) {
  // executeOnChildStorage

  Buffer child_storage_key(8, 'l');
  Buffer key(8, 'k');

  // logic

  WasmOffset offset = 4;

  // 'func' (lambda)
  auto &param = GetParam();
  EXPECT_CALL(*trie_child_storage_batch_, tryGetMock(key.view()))
      .WillOnce(Return(param));

  // results

  auto value_span = memory_.allocate2(2);
  auto call = [&] {
    return child_storage_extension_->ext_default_child_storage_read_version_1(
        memory_[child_storage_key], memory_[key], value_span.combine(), offset);
  };
  if (GetParam().has_error()) {
    ASSERT_THROW({ call(); }, std::runtime_error);
  } else {
    auto &param = GetParam().value();
    using Result = std::optional<uint32_t>;
    Result result;
    if (param) {
      result = param->size() - offset;
    }
    ASSERT_EQ(memory_.decode<Result>(call()), result);
    if (param) {
      auto n = std::min<size_t>(value_span.size, param->size());
      ASSERT_EQ(memory_.view(value_span.ptr, n).value(),
                SpanAdl{param->view(offset, n)});
    }
  }
}

/**
 * @given child storage key, key, value
 * @when invoke ext_default_child_storage_set_version_1
 * @then upon success: (over)write a value into child storage
 * upon failure: outcome::failure
 */
TEST_P(VoidOutcomeParameterizedTest, SetTest) {
  // executeOnChildStorage
  Buffer child_storage_key(8, 'l');
  Buffer key(8, 'k');

  if (GetParam()) {
    RootHash new_child_root = "123456"_hash256;
    EXPECT_CALL(*trie_child_storage_batch_, commit(_))
        .WillOnce(Return(new_child_root));
  }

  // logic
  Buffer value(8, 'v');
  EXPECT_CALL(*trie_child_storage_batch_, put(key.view(), value))
      .WillOnce(Return(GetParam()));

  child_storage_extension_->ext_default_child_storage_set_version_1(
      memory_[child_storage_key], memory_[key], memory_[value]);
  if (GetParam()) {
    child_storage_extension_->ext_default_child_storage_root_version_1(
        memory_[child_storage_key]);
  }
}

/**
 * @given child storage key, key
 * @when invoke ext_default_child_storage_clear_version_1
 * @then upon success: remove a value from child storage
 * upon failure: outcome::failure
 */
TEST_P(VoidOutcomeParameterizedTest, ClearTest) {
  // executeOnChildStorage
  Buffer child_storage_key(8, 'l');
  Buffer key(8, 'k');

  if (GetParam()) {
    RootHash new_child_root = "123456"_hash256;
    EXPECT_CALL(*trie_child_storage_batch_, commit(_))
        .WillOnce(Return(new_child_root));
  }

  // logic
  EXPECT_CALL(*trie_child_storage_batch_, remove(key.view()))
      .WillOnce(Return(GetParam()));

  child_storage_extension_->ext_default_child_storage_clear_version_1(
      memory_[child_storage_key], memory_[key]);
  if (GetParam()) {
    child_storage_extension_->ext_default_child_storage_root_version_1(
        memory_[child_storage_key]);
  }
}

/**
 * @note ext_default_child_storage_storage_kill_version_1 is a subvariant
 * of ext_default_child_storage_clear_prefix_version_1 with empty prefix
 * @given child storage key, prefix
 * @when invoke ext_default_child_storage_clear_prefix_version_1
 * @then remove all values with prefix from child storage. If child storage is
 * empty as a result, it will be pruned later.
 */
TEST_F(ChildStorageExtensionTest, ClearPrefixKillTest) {
  // executeOnChildStorage
  Buffer child_storage_key(8, 'l');
  Buffer prefix(8, 'p');

  RootHash new_child_root = "123456"_hash256;
  EXPECT_CALL(*trie_child_storage_batch_, commit(_))
      .WillOnce(Return(new_child_root));

  // logic
  std::optional<uint64_t> limit = std::nullopt;
  EXPECT_CALL(*trie_child_storage_batch_, clearPrefix(prefix.view(), limit))
      .WillOnce(Return(outcome::success(std::make_tuple(true, 33u))));

  child_storage_extension_->ext_default_child_storage_clear_prefix_version_1(
      memory_[child_storage_key], memory_[prefix]);
  child_storage_extension_->ext_default_child_storage_root_version_1(
      memory_[child_storage_key]);
}

/**
 * @given child storage key, key
 * @when invoke ext_default_child_storage_next_key_version_1
 * @then return next key after given one, in lexicographical order
 */
TEST_F(ChildStorageExtensionTest, NextKeyTest) {
  Buffer child_storage_key(8, 'l');
  Buffer key(8, 'k');
  EXPECT_CALL(*trie_child_storage_batch_, trieCursor())
      .WillOnce(Invoke([&key]() {
        auto cursor = std::make_unique<PolkadotTrieCursorMock>();
        EXPECT_CALL(*cursor, seekUpperBound(key.view()))
            .WillOnce(Return(outcome::success()));
        EXPECT_CALL(*cursor, key())
            .WillOnce(Return(std::make_optional<Buffer>("12345"_buf)));
        return cursor;
      }));

  memory_[child_storage_extension_
              ->ext_default_child_storage_next_key_version_1(
                  memory_[child_storage_key], memory_[key])];
}

/**
 * @given child storage key
 * @when invoke ext_default_child_storage_root_version_1
 * @then returns new child root value
 */
TEST_F(ChildStorageExtensionTest, RootTest) {
  Buffer child_storage_key(8, 'l');

  RootHash new_child_root = "123456"_hash256;
  EXPECT_CALL(*trie_child_storage_batch_, commit(_))
      .WillOnce(Return(new_child_root));

  ASSERT_EQ(memory_[child_storage_extension_
                        ->ext_default_child_storage_root_version_1(
                            memory_[child_storage_key])],
            new_child_root);
}

/**
 * @given child storage key, key
 * @when invoke ext_default_child_storage_exists_version_1
 * @then return 1 if value exists, 0 otherwise
 */
TEST_P(BoolOutcomeParameterizedTest, ExistsTest) {
  // executeOnChildStorage
  Buffer child_storage_key(8, 'l');
  Buffer key(8, 'k');

  // logic
  EXPECT_CALL(*trie_child_storage_batch_, contains(key.view()))
      .WillOnce(Return(GetParam()));

  ASSERT_EQ(
      GetParam() ? GetParam().value() : 0,
      child_storage_extension_->ext_default_child_storage_exists_version_1(
          memory_[child_storage_key], memory_[key]));
}

INSTANTIATE_TEST_SUITE_P(Instance,
                         VoidOutcomeParameterizedTest,
                         ::testing::Values<outcome::result<void>>(
                             /// success case
                             outcome::success(),
                             /// failure with arbitrary error code
                             outcome::failure(testutil::DummyError::ERROR))
                         // empty argument for the macro
);

INSTANTIATE_TEST_SUITE_P(
    Instance,
    ReadOutcomeParameterizedTest,
    ::testing::Values<outcome::result<std::optional<Buffer>>>(
        /// success case
        outcome::success(std::make_optional("08070605040302"_buf)),
        /// not found
        outcome::success(std::nullopt),
        /// failure with arbitrary error code
        outcome::failure(testutil::DummyError::ERROR))
    // empty argument for the macro
);

INSTANTIATE_TEST_SUITE_P(Instance,
                         BoolOutcomeParameterizedTest,
                         ::testing::Values<outcome::result<bool>>(
                             /// success case
                             outcome::success(true),
                             /// not found
                             outcome::success(false),
                             /// failure with arbitrary error code
                             outcome::failure(testutil::DummyError::ERROR))
                         // empty argument for the macro
);
