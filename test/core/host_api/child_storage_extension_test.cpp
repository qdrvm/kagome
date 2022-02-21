/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/child_storage_extension.hpp"

#include <gtest/gtest.h>
#include <optional>
#include <scale/scale.hpp>

#include "common/monadic_utils.h"
#include "mock/core/runtime/memory_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/changes_trie/changes_tracker_mock.hpp"
#include "mock/core/storage/trie/polkadot_trie_cursor_mock.h"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "runtime/ptr_size.hpp"
#include "scale/encode_append.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "storage/trie/types.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/outcome/dummy_error.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Buffer;
using kagome::common::BufferConstRef;
using kagome::host_api::ChildStorageExtension;
using kagome::runtime::Memory;
using kagome::runtime::MemoryMock;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::PtrSize;
using kagome::runtime::TrieStorageProviderMock;
using kagome::runtime::WasmOffset;
using kagome::runtime::WasmPointer;
using kagome::runtime::WasmSize;
using kagome::runtime::WasmSpan;
using kagome::storage::trie::PersistentTrieBatchMock;
using kagome::storage::trie::PolkadotTrieCursorMock;
using kagome::storage::trie::RootHash;
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
    trie_child_storage_batch_ = std::make_shared<PersistentTrieBatchMock>();
    trie_batch_ = std::make_shared<PersistentTrieBatchMock>();
    storage_provider_ = std::make_shared<TrieStorageProviderMock>();
    EXPECT_CALL(*storage_provider_, getCurrentBatch())
        .WillRepeatedly(Return(trie_batch_));
    EXPECT_CALL(*storage_provider_, isCurrentlyPersistent())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*storage_provider_, getChildBatchAt(_))
        .WillRepeatedly(Return(std::static_pointer_cast<
                               kagome::storage::trie::PersistentTrieBatch>(
            trie_child_storage_batch_)));
    EXPECT_CALL(*storage_provider_, clearChildBatches())
        .WillRepeatedly(Return());
    EXPECT_CALL(*storage_provider_, tryGetPersistentBatch())
        .WillRepeatedly(Return(std::make_optional(
            std::static_pointer_cast<
                kagome::storage::trie::PersistentTrieBatch>(trie_batch_))));
    memory_provider_ = std::make_shared<MemoryProviderMock>();
    memory_ = std::make_shared<MemoryMock>();
    EXPECT_CALL(*memory_provider_, getCurrentMemory())
        .WillRepeatedly(
            Return(std::optional<std::reference_wrapper<Memory>>(*memory_)));
    child_storage_extension_ = std::make_shared<ChildStorageExtension>(
        storage_provider_, memory_provider_);
  }

 protected:
  std::shared_ptr<PersistentTrieBatchMock> trie_child_storage_batch_;
  std::shared_ptr<PersistentTrieBatchMock> trie_batch_;
  std::shared_ptr<TrieStorageProviderMock> storage_provider_;
  std::shared_ptr<MemoryMock> memory_;
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

  WasmPointer child_storage_key_pointer = 42;
  WasmSize child_storage_key_size = 42;
  WasmSpan child_storage_key_span =
      PtrSize(child_storage_key_pointer, child_storage_key_size).combine();
  Buffer child_storage_key(8, 'l');
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();
  Buffer key(8, 'k');
  EXPECT_CALL(*memory_,
              loadN(child_storage_key_pointer, child_storage_key_size))
      .WillOnce(Return(child_storage_key));
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));

  Buffer prefixed_child_storage_key;
  if (GetParam()) {
    RootHash new_child_root = "123456"_hash256;
    Buffer new_child_root_buffer{scale::encode(new_child_root).value()};
    EXPECT_CALL(*trie_child_storage_batch_, commit())
        .WillOnce(Return(new_child_root));

    prefixed_child_storage_key =
        Buffer{kagome::storage::kChildStorageDefaultPrefix}.putBuffer(
            child_storage_key);
    EXPECT_CALL(*trie_batch_,
                put(prefixed_child_storage_key.view(), new_child_root_buffer))
        .WillOnce(Return(outcome::success()));
  }

  // logic

  WasmPointer value_pointer = 44;
  WasmSize value_size = 44;
  WasmSpan value_span = PtrSize(value_pointer, value_size).combine();

  std::vector<uint8_t> encoded_opt_value;
  if (GetParam()) {
    encoded_opt_value = scale::encode(GetParam().value()).value();
  }

  // 'func' (lambda)

  auto &param = GetParam();
  auto param_ref = kagome::common::map_result_optional(
      param, [](auto &v) { return std::cref(v); });

  EXPECT_CALL(*trie_child_storage_batch_, tryGet(key.view()))
      .WillOnce(Return(param_ref));

  // results
  if (GetParam().has_error()) {
    ASSERT_THROW(
        {
          child_storage_extension_->ext_default_child_storage_get_version_1(
              child_storage_key_span, key_span);
        },
        std::runtime_error);
  } else {
    EXPECT_CALL(*memory_,
                storeBuffer(gsl::span<const uint8_t>(encoded_opt_value)))
        .WillOnce(Return(value_span));

    ASSERT_EQ(value_span,
              child_storage_extension_->ext_default_child_storage_get_version_1(
                  child_storage_key_span, key_span));
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

  WasmPointer child_storage_key_pointer = 42;
  WasmSize child_storage_key_size = 42;
  WasmSpan child_storage_key_span =
      PtrSize(child_storage_key_pointer, child_storage_key_size).combine();
  Buffer child_storage_key(8, 'l');
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();
  Buffer key(8, 'k');
  EXPECT_CALL(*memory_,
              loadN(child_storage_key_pointer, child_storage_key_size))
      .WillOnce(Return(child_storage_key));
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));

  Buffer prefixed_child_storage_key;
  if (GetParam()) {
    RootHash new_child_root = "123456"_hash256;
    Buffer new_child_root_buffer{scale::encode(new_child_root).value()};
    EXPECT_CALL(*trie_child_storage_batch_, commit())
        .WillOnce(Return(new_child_root));

    prefixed_child_storage_key =
        Buffer{kagome::storage::kChildStorageDefaultPrefix}.putBuffer(
            child_storage_key);
    EXPECT_CALL(*trie_batch_,
                put(prefixed_child_storage_key.view(), new_child_root_buffer))
        .WillOnce(Return(outcome::success()));
  }

  // logic

  WasmPointer value_pointer = 44;
  WasmSize value_size = 44;
  WasmSpan value_span = PtrSize(value_pointer, value_size).combine();
  auto encoded_result =
      scale::encode<std::optional<uint32_t>>(std::nullopt).value();

  WasmOffset offset = 4;
  Buffer offset_value_data;

  if (GetParam() && GetParam().value()) {
    auto &param = GetParam().value().value();
    offset_value_data = param.subbuffer(offset);
    ASSERT_EQ(offset_value_data.size(), param.size() - offset);
    EXPECT_OUTCOME_TRUE(
        encoded_opt_offset_val_size,
        scale::encode(std::make_optional<uint32_t>(offset_value_data.size())));
    encoded_result = encoded_opt_offset_val_size;
    EXPECT_CALL(
        *memory_,
        storeBuffer(value_pointer, gsl::span<const uint8_t>(offset_value_data)))
        .WillOnce(Return());
  }

  // 'func' (lambda)

  auto &param = GetParam();
  EXPECT_CALL(*trie_child_storage_batch_, tryGet(key.view()))
      .WillOnce(Return([&]() -> outcome::result<std::optional<BufferConstRef>> {
        if (param.has_error()) return param.as_failure();
        auto &opt = param.value();
        if (opt) return std::cref(opt.value());
        return std::nullopt;
      }()));

  // results

  if (GetParam().has_error()) {
    ASSERT_THROW(
        {
          child_storage_extension_->ext_default_child_storage_read_version_1(
              child_storage_key_span, key_span, value_span, offset);
        },
        std::runtime_error);
  } else {
    WasmSpan res_wasm_span = 1337;
    EXPECT_CALL(*memory_, storeBuffer(gsl::span<const uint8_t>(encoded_result)))
        .WillOnce(Return(res_wasm_span));

    ASSERT_EQ(
        res_wasm_span,
        child_storage_extension_->ext_default_child_storage_read_version_1(
            child_storage_key_span, key_span, value_span, offset));
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
  WasmPointer child_storage_key_pointer = 42;
  WasmSize child_storage_key_size = 42;
  WasmSpan child_storage_key_span =
      PtrSize(child_storage_key_pointer, child_storage_key_size).combine();
  Buffer child_storage_key(8, 'l');
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();
  Buffer key(8, 'k');
  EXPECT_CALL(*memory_,
              loadN(child_storage_key_pointer, child_storage_key_size))
      .WillOnce(Return(child_storage_key));
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));

  Buffer prefixed_child_storage_key;
  if (GetParam()) {
    RootHash new_child_root = "123456"_hash256;
    Buffer new_child_root_buffer{scale::encode(new_child_root).value()};
    EXPECT_CALL(*trie_child_storage_batch_, commit())
        .WillOnce(Return(new_child_root));

    prefixed_child_storage_key =
        Buffer{kagome::storage::kChildStorageDefaultPrefix}.putBuffer(
            child_storage_key);
    EXPECT_CALL(*trie_batch_,
                put(prefixed_child_storage_key.view(), new_child_root_buffer))
        .WillOnce(Return(outcome::success()));
  }

  // logic
  WasmPointer value_pointer = 44;
  WasmSize value_size = 44;
  WasmSpan value_span = PtrSize(value_pointer, value_size).combine();
  Buffer value(8, 'v');
  EXPECT_CALL(*memory_, loadN(value_pointer, value_size))
      .WillOnce(Return(value));
  EXPECT_CALL(*trie_child_storage_batch_, put(key.view(), value))
      .WillOnce(Return(GetParam()));

  child_storage_extension_->ext_default_child_storage_set_version_1(
      child_storage_key_span, key_span, value_span);
}

/**
 * @given child storage key, key
 * @when invoke ext_default_child_storage_clear_version_1
 * @then upon success: remove a value from child storage
 * upon failure: outcome::failure
 */
TEST_P(VoidOutcomeParameterizedTest, ClearTest) {
  // executeOnChildStorage
  WasmPointer child_storage_key_pointer = 42;
  WasmSize child_storage_key_size = 42;
  WasmSpan child_storage_key_span =
      PtrSize(child_storage_key_pointer, child_storage_key_size).combine();
  Buffer child_storage_key(8, 'l');
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();
  Buffer key(8, 'k');
  EXPECT_CALL(*memory_,
              loadN(child_storage_key_pointer, child_storage_key_size))
      .WillOnce(Return(child_storage_key));
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));

  if (GetParam()) {
    RootHash new_child_root = "123456"_hash256;
    Buffer new_child_root_buffer{scale::encode(new_child_root).value()};
    EXPECT_CALL(*trie_child_storage_batch_, commit())
        .WillOnce(Return(new_child_root));
    Buffer prefixed_child_storage_key =
        Buffer{kagome::storage::kChildStorageDefaultPrefix}.putBuffer(
            child_storage_key);
    EXPECT_CALL(*trie_batch_,
                put(prefixed_child_storage_key.view(), new_child_root_buffer))
        .WillOnce(Return(outcome::success()));
  }

  // logic
  EXPECT_CALL(*trie_child_storage_batch_, remove(key.view()))
      .WillOnce(Return(GetParam()));

  child_storage_extension_->ext_default_child_storage_clear_version_1(
      child_storage_key_span, key_span);
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
  WasmPointer child_storage_key_pointer = 42;
  WasmSize child_storage_key_size = 42;
  WasmSpan child_storage_key_span =
      PtrSize(child_storage_key_pointer, child_storage_key_size).combine();
  Buffer child_storage_key(8, 'l');
  WasmPointer prefix_pointer = 40;
  WasmSize prefix_size = 40;
  WasmSpan prefix_span = PtrSize(prefix_pointer, prefix_size).combine();
  Buffer prefix(8, 'p');
  EXPECT_CALL(*memory_,
              loadN(child_storage_key_pointer, child_storage_key_size))
      .WillOnce(Return(child_storage_key));
  EXPECT_CALL(*memory_, loadN(prefix_pointer, prefix_size))
      .WillOnce(Return(prefix));

  RootHash new_child_root = "123456"_hash256;
  Buffer new_child_root_buffer{scale::encode(new_child_root).value()};
  EXPECT_CALL(*trie_child_storage_batch_, commit())
      .WillOnce(Return(new_child_root));

  Buffer prefixed_child_storage_key =
      Buffer{kagome::storage::kChildStorageDefaultPrefix}.putBuffer(
          child_storage_key);
  EXPECT_CALL(*trie_batch_,
              put(prefixed_child_storage_key.view(), new_child_root_buffer))
      .WillOnce(Return(outcome::success()));

  // logic
  std::optional<uint64_t> limit = std::nullopt;
  EXPECT_CALL(*trie_child_storage_batch_, clearPrefix(prefix.view(), limit))
      .WillOnce(Return(outcome::success(std::make_tuple(true, 33u))));

  child_storage_extension_->ext_default_child_storage_clear_prefix_version_1(
      child_storage_key_span, prefix_span);
}

/**
 * @given child storage key, key
 * @when invoke ext_default_child_storage_next_key_version_1
 * @then return next key after given one, in lexicographical order
 */
TEST_F(ChildStorageExtensionTest, NextKeyTest) {
  WasmPointer child_storage_key_pointer = 42;
  WasmSize child_storage_key_size = 42;
  WasmSpan child_storage_key_span =
      PtrSize(child_storage_key_pointer, child_storage_key_size).combine();
  Buffer child_storage_key(8, 'l');
  Buffer prefixed_child_storage_key =
      Buffer{kagome::storage::kChildStorageDefaultPrefix}.putBuffer(
          child_storage_key);
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();
  Buffer key(8, 'k');
  EXPECT_CALL(*memory_,
              loadN(child_storage_key_pointer, child_storage_key_size))
      .WillOnce(Return(child_storage_key));
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*trie_child_storage_batch_, trieCursor())
      .WillOnce(Invoke([&key]() {
        auto cursor = std::make_unique<PolkadotTrieCursorMock>();
        EXPECT_CALL(*cursor, seekUpperBound(key.view()))
            .WillOnce(Return(outcome::success()));
        EXPECT_CALL(*cursor, key())
            .WillOnce(Return(std::make_optional<Buffer>("12345"_buf)));
        return cursor;
      }));

  WasmPointer res_pointer = 44;
  WasmSize res_size = 44;
  WasmSpan res_span = PtrSize(res_pointer, res_size).combine();
  EXPECT_CALL(*memory_, storeBuffer(_)).WillOnce(Return(res_span));

  ASSERT_EQ(
      res_span,
      child_storage_extension_->ext_default_child_storage_next_key_version_1(
          child_storage_key_span, key_span));
}

/**
 * @given child storage key
 * @when invoke ext_default_child_storage_root_version_1
 * @then returns new child root value
 */
TEST_F(ChildStorageExtensionTest, RootTest) {
  WasmPointer child_storage_key_pointer = 42;
  WasmSize child_storage_key_size = 42;
  WasmSpan child_storage_key_span =
      PtrSize(child_storage_key_pointer, child_storage_key_size).combine();
  Buffer child_storage_key(8, 'l');
  Buffer prefixed_child_storage_key =
      Buffer{kagome::storage::kChildStorageDefaultPrefix}.putBuffer(
          child_storage_key);
  EXPECT_CALL(*memory_,
              loadN(child_storage_key_pointer, child_storage_key_size))
      .WillOnce(Return(child_storage_key));

  RootHash new_child_root = "123456"_hash256;
  WasmPointer new_child_root_ptr = 1984;
  WasmPointer new_child_root_size = 12;
  WasmSpan new_child_root_span =
      PtrSize(new_child_root_ptr, new_child_root_size).combine();
  EXPECT_CALL(*trie_child_storage_batch_, commit())
      .WillOnce(Return(new_child_root));
  EXPECT_CALL(*memory_, storeBuffer(gsl::span<const uint8_t>(new_child_root)))
      .WillOnce(Return(new_child_root_span));

  ASSERT_EQ(new_child_root_span,
            child_storage_extension_->ext_default_child_storage_root_version_1(
                child_storage_key_span));
}

/**
 * @given child storage key, key
 * @when invoke ext_default_child_storage_exists_version_1
 * @then return 1 if value exists, 0 otherwise
 */
TEST_P(BoolOutcomeParameterizedTest, ExistsTest) {
  // executeOnChildStorage
  WasmPointer child_storage_key_pointer = 42;
  WasmSize child_storage_key_size = 42;
  WasmSpan child_storage_key_span =
      PtrSize(child_storage_key_pointer, child_storage_key_size).combine();
  Buffer child_storage_key(8, 'l');
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();
  Buffer key(8, 'k');
  EXPECT_CALL(*memory_,
              loadN(child_storage_key_pointer, child_storage_key_size))
      .WillOnce(Return(child_storage_key));
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));

  Buffer prefixed_child_storage_key;
  if (GetParam()) {
    RootHash new_child_root = "123456"_hash256;
    Buffer new_child_root_buffer{scale::encode(new_child_root).value()};
    EXPECT_CALL(*trie_child_storage_batch_, commit())
        .WillOnce(Return(new_child_root));

    prefixed_child_storage_key =
        Buffer{kagome::storage::kChildStorageDefaultPrefix}.putBuffer(
            child_storage_key);
    EXPECT_CALL(*trie_batch_,
                put(prefixed_child_storage_key.view(), new_child_root_buffer))
        .WillOnce(Return(outcome::success()));
  }

  // logic
  EXPECT_CALL(*trie_child_storage_batch_, contains(key.view()))
      .WillOnce(Return(GetParam()));

  ASSERT_EQ(
      GetParam() ? GetParam().value() : 0,
      child_storage_extension_->ext_default_child_storage_exists_version_1(
          child_storage_key_span, key_span));
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