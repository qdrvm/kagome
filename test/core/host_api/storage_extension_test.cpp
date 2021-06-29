/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/storage_extension.hpp"

#include <gtest/gtest.h>
#include <scale/scale.hpp>

#include "mock/core/runtime/memory_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/changes_trie/changes_tracker_mock.hpp"
#include "mock/core/storage/trie/polkadot_trie_cursor_mock.h"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "runtime/ptr_size.hpp"
#include "scale/encode_append.hpp"
#include "storage/changes_trie/changes_trie_config.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/outcome/dummy_error.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::host_api::StorageExtension;
using kagome::runtime::TrieStorageProviderMock;
using kagome::runtime::PtrSize;
using kagome::runtime::WasmOffset;
using kagome::runtime::WasmPointer;
using kagome::runtime::WasmSize;
using kagome::runtime::WasmSpan;
using kagome::storage::changes_trie::ChangesTrackerMock;
using kagome::storage::trie::EphemeralTrieBatchMock;
using kagome::storage::trie::PersistentTrieBatchMock;
using kagome::storage::trie::PolkadotTrieCursorMock;
using kagome::runtime::MemoryMock;
using kagome::runtime::Memory;
using kagome::runtime::MemoryProviderMock;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

class StorageExtensionTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    trie_batch_ = std::make_shared<PersistentTrieBatchMock>();
    storage_provider_ = std::make_shared<TrieStorageProviderMock>();
    EXPECT_CALL(*storage_provider_, getCurrentBatch())
        .WillRepeatedly(Return(trie_batch_));
    EXPECT_CALL(*storage_provider_, isCurrentlyPersistent())
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*storage_provider_, tryGetPersistentBatch())
        .WillRepeatedly(Return(boost::make_optional(
            std::static_pointer_cast<
                kagome::storage::trie::PersistentTrieBatch>(trie_batch_))));
    memory_provider_ = std::make_shared<MemoryProviderMock>();
    memory_ = std::make_shared<MemoryMock>();
    EXPECT_CALL(*memory_provider_, getCurrentMemory())
        .WillRepeatedly(Return(boost::optional<std::shared_ptr<Memory>>(memory_)));
    changes_tracker_ = std::make_shared<ChangesTrackerMock>();
    storage_extension_ = std::make_shared<StorageExtension>(
        storage_provider_, memory_provider_, changes_tracker_);
  }

 protected:
  std::shared_ptr<PersistentTrieBatchMock> trie_batch_;
  std::shared_ptr<TrieStorageProviderMock> storage_provider_;
  std::shared_ptr<MemoryMock> memory_;
  std::shared_ptr<MemoryProviderMock> memory_provider_;
  std::shared_ptr<StorageExtension> storage_extension_;
  std::shared_ptr<ChangesTrackerMock> changes_tracker_;

  constexpr static uint32_t kU32Max = std::numeric_limits<uint32_t>::max();
};

/// For the tests where it is needed to check a valid behaviour no matter if
/// success or failure was returned by outcome::result
class OutcomeParameterizedTest
    : public StorageExtensionTest,
      public ::testing::WithParamInterface<outcome::result<void>> {};

struct EnumeratedTrieRootTestCase {
  std::list<Buffer> values;
  Buffer trie_root_buf;
};

/// For tests that operate over a collection of buffers
class BuffersParametrizedTest
    : public StorageExtensionTest,
      public testing::WithParamInterface<EnumeratedTrieRootTestCase> {};

/**
 * @given prefix_pointer with prefix_length
 * @when ext_clear_prefix is invoked on StorageExtension with given prefix
 * @then prefix is loaded from the memory @and clearPrefix is invoked on storage
 */
TEST_F(StorageExtensionTest, ClearPrefixTest) {
  WasmPointer prefix_pointer = 42;
  WasmSize prefix_size = 42;
  Buffer prefix(8, 'p');

  EXPECT_CALL(*memory_, loadN(prefix_pointer, prefix_size))
      .WillOnce(Return(prefix));
  EXPECT_CALL(*trie_batch_, clearPrefix(prefix))
      .Times(1)
      .WillOnce(Return(outcome::success()));

  storage_extension_->ext_storage_clear_prefix_version_1(
      PtrSize{prefix_pointer, prefix_size}.combine());
}

/**
 * @given key_pointer and key_size
 * @when ext_clear_storage is invoked on StorageExtension with given key
 * @then key is loaded from the memory @and del is invoked on storage
 */
TEST_P(OutcomeParameterizedTest, ClearStorageTest) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');

  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*trie_batch_, remove(key)).WillOnce(Return(GetParam()));

  storage_extension_->ext_storage_clear_version_1(
      PtrSize{key_pointer, key_size}.combine());
}

/**
 * @given key pointer and key size
 * @when ext_exists_storage is invoked on StorageExtension with given key
 * @then result is the same as result of contains on given key
 */
TEST_F(StorageExtensionTest, ExistsStorageTest) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');

  /// result of contains method on db
  WasmSize contains = 1;

  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*trie_batch_, contains(key)).WillOnce(Return(contains));

  ASSERT_EQ(contains,
            storage_extension_->ext_storage_exists_version_1(
                PtrSize{key_pointer, key_size}.combine()));
}

/**
 * @given a trie key address in WASM memory, to which there is a key
 * lexicographically greater
 * @when using ext_storage_next_key_version_1 to obtain the next key
 * @then an address of the next key is returned
 */
TEST_F(StorageExtensionTest, NextKey) {
  // as wasm logic is mocked, it is okay that key and next_key 'intersect' in
  // wasm memory
  WasmPointer key_pointer = 43;
  WasmSize key_size = 8;
  Buffer key(key_size, 'k');
  WasmPointer next_key_pointer = 44;
  WasmSize next_key_size = 9;
  Buffer expected_next_key(next_key_size, 'k');

  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));

  EXPECT_CALL(*trie_batch_, trieCursor())
      .WillOnce(Invoke([&key, &expected_next_key]() {
        auto cursor = std::make_unique<PolkadotTrieCursorMock>();
        EXPECT_CALL(*cursor, seekUpperBound(key))
            .WillOnce(Return(outcome::success()));
        EXPECT_CALL(*cursor, key()).WillOnce(Return(expected_next_key));
        return cursor;
      }));

  auto expected_key_span = PtrSize{next_key_pointer, next_key_size}.combine();
  EXPECT_CALL(*memory_, storeBuffer(_))
      .WillOnce(Invoke([&expected_next_key,
                        &expected_key_span](auto &&buffer) -> WasmSpan {
        EXPECT_OUTCOME_TRUE(
            key_opt, kagome::scale::decode<boost::optional<Buffer>>(buffer));
        EXPECT_TRUE(key_opt.has_value());
        EXPECT_EQ(key_opt.value(), expected_next_key);
        return expected_key_span;
      }));

  auto next_key_span = storage_extension_->ext_storage_next_key_version_1(
      PtrSize{key_pointer, key_size}.combine());
  ASSERT_EQ(expected_key_span, next_key_span);
}

/**
 * @given a trie key address in WASM memory, to which there is no key
 * lexicographically greater
 * @when using ext_storage_next_key_version_1 to obtain the next key
 * @then an address of none value is returned
 */
TEST_F(StorageExtensionTest, NextKeyLastKey) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 8;
  Buffer key(key_size, 'k');

  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));

  EXPECT_CALL(*trie_batch_, trieCursor()).WillOnce(Invoke([&key]() {
    auto cursor = std::make_unique<PolkadotTrieCursorMock>();
    EXPECT_CALL(*cursor, seekUpperBound(key))
        .WillOnce(Return(outcome::success()));
    EXPECT_CALL(*cursor, key()).WillOnce(Return(boost::none));
    return cursor;
  }));

  EXPECT_CALL(*memory_, storeBuffer(_))
      .WillOnce(Invoke([](auto &&buffer) -> WasmSpan {
        EXPECT_OUTCOME_TRUE(
            key_opt, kagome::scale::decode<boost::optional<Buffer>>(buffer));
        EXPECT_EQ(key_opt, boost::none);
        return 0;  // don't need the result
      }));

  storage_extension_->ext_storage_next_key_version_1(
      PtrSize{key_pointer, key_size}.combine());
}

/**
 * @given a trie key address in WASM memory, which is not present in the storage
 * @when using ext_storage_next_key_version_1 to obtain the next key
 * @then an invalid address is returned
 */
TEST_F(StorageExtensionTest, NextKeyEmptyTrie) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 8;
  Buffer key(key_size, 'k');

  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));

  EXPECT_CALL(*trie_batch_, trieCursor()).WillOnce(Invoke([&key]() {
    auto cursor = std::make_unique<PolkadotTrieCursorMock>();
    EXPECT_CALL(*cursor, seekUpperBound(key))
        .WillOnce(Return(outcome::success()));
    EXPECT_CALL(*cursor, key()).WillOnce(Return(boost::none));
    return cursor;
  }));

  EXPECT_CALL(*memory_, storeBuffer(_))
      .WillOnce(Invoke([](auto &&buffer) -> WasmSpan {
        EXPECT_OUTCOME_TRUE(
            key_opt, kagome::scale::decode<boost::optional<Buffer>>(buffer));
        EXPECT_EQ(key_opt, boost::none);
        return 0;
      }));

  storage_extension_->ext_storage_next_key_version_1(
      PtrSize{key_pointer, key_size}.combine());
}

/**
 * @given key_pointer, key_size, value_ptr, value_size
 * @when ext_set_storage is invoked on given key and value
 * @then provided key and value are put to db
 */
TEST_P(OutcomeParameterizedTest, SetStorageTest) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');

  WasmPointer value_pointer = 42;
  WasmSize value_size = 41;
  Buffer value(8, 'v');

  // expect key and value were loaded
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*memory_, loadN(value_pointer, value_size))
      .WillOnce(Return(value));

  // expect key-value pair was put to db
  EXPECT_CALL(*trie_batch_, put(key, value)).WillOnce(Return(GetParam()));

  storage_extension_->ext_storage_set_version_1(
      PtrSize{key_pointer, key_size}.combine(),
      PtrSize{value_pointer, value_size}.combine());
}

/**
 * @given key_pointer, key_size, value_ptr, value_size
 * @when ext_storage_set_version_1 is invoked on given key and value
 * @then provided key and value are put to db
 */
TEST_P(OutcomeParameterizedTest, ExtStorageSetV1Test) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');

  WasmPointer value_pointer = 42;
  WasmSize value_size = 41;
  Buffer value(8, 'v');

  // expect key and value were loaded
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*memory_, loadN(value_pointer, value_size))
      .WillOnce(Return(value));

  // expect key-value pair was put to db
  EXPECT_CALL(*trie_batch_, put(key, value)).WillOnce(Return(GetParam()));

  storage_extension_->ext_storage_set_version_1(
      PtrSize(key_pointer, key_size).combine(),
      PtrSize(value_pointer, value_size).combine());
}

/**
 * @given key, value, offset
 * @when ext_storage_read_version_1 is invoked on given key and value
 * @then data read from db with given key
 */
TEST_P(OutcomeParameterizedTest, StorageReadTest) {
  PtrSize key(43, 43);
  Buffer key_data(key.size, 'k');
  PtrSize value(42, 41);
  Buffer value_data(value.size, 'v');
  WasmOffset offset = 4;
  Buffer offset_value_data = value_data.subbuffer(offset);
  ASSERT_EQ(offset_value_data.size(), value_data.size() - offset);
  EXPECT_OUTCOME_TRUE(encoded_opt_offset_val_size,
                      kagome::scale::encode(boost::make_optional<uint32_t>(
                          offset_value_data.size())));
  WasmSpan res_wasm_span = 1337;

  // expect key loaded, than data stored
  EXPECT_CALL(*memory_, loadN(key.ptr, key.size))
      .WillOnce(Return(key_data));
  EXPECT_CALL(*storage_provider_, getCurrentBatch())
      .WillOnce(Return(trie_batch_));
  EXPECT_CALL(*trie_batch_, get(key_data)).WillOnce(Return(value_data));
  EXPECT_CALL(
      *memory_,
      storeBuffer(value.ptr, gsl::span<const uint8_t>(offset_value_data)));
  EXPECT_CALL(
      *memory_,
      storeBuffer(gsl::span<const uint8_t>(encoded_opt_offset_val_size)))
      .WillOnce(Return(res_wasm_span));

  ASSERT_EQ(res_wasm_span,
            storage_extension_->ext_storage_read_version_1(
                key.combine(), value.combine(), offset));
}

INSTANTIATE_TEST_CASE_P(Instance,
                        OutcomeParameterizedTest,
                        ::testing::Values<outcome::result<void>>(
                            /// success case
                            outcome::success(),
                            /// failure with arbitrary error code
                            outcome::failure(testutil::DummyError::ERROR)),
                        // empty argument for the macro
);

TEST_F(StorageExtensionTest, ExtStorageAppendTest) {
  // @given key and two values
  PtrSize key(43, 43);
  Buffer key_data(key.size, 'k');

  Buffer value_data1(42, '1');
  Buffer value_data1_encoded{kagome::scale::encode(value_data1).value()};
  PtrSize value1(42, value_data1_encoded.size());

  Buffer value_data2(43, '2');
  Buffer value_data2_encoded{kagome::scale::encode(value_data2).value()};
  PtrSize value2(45, value_data2_encoded.size());

  // @given wasm memory that can provide these key and values
  EXPECT_CALL(*memory_, loadN(key.ptr, key.size))
      .WillRepeatedly(Return(key_data));
  EXPECT_CALL(*memory_, loadN(value1.ptr, value1.size))
      .WillOnce(Return(value_data1_encoded));
  EXPECT_CALL(*memory_, loadN(value2.ptr, value2.size))
      .WillOnce(Return(value_data2_encoded));

  std::vector<kagome::scale::EncodeOpaqueValue> vals;
  Buffer vals_encoded;
  {
    // @when there is no value by given key in trie
    EXPECT_CALL(*trie_batch_, get(key_data))
        .WillOnce(Return(outcome::failure(boost::system::error_code{})));

    // @then storage is inserted by scale encoded vector containing
    // EncodeOpaqueValue with value1
    vals.push_back(
        kagome::scale::EncodeOpaqueValue{value_data1_encoded.asVector()});
    vals_encoded = Buffer(kagome::scale::encode(vals).value());
    EXPECT_CALL(*trie_batch_, put_rvalueHack(key_data, vals_encoded))
        .WillOnce(Return(outcome::success()));

    storage_extension_->ext_storage_append_version_1(key.combine(),
                                                     value1.combine());
  }

  {
    // @when there is a value by given key (inserted above)
    EXPECT_CALL(*trie_batch_, get(key_data)).WillOnce(Return(vals_encoded));

    // @then storage is inserted by scale encoded vector containing two
    // EncodeOpaqueValues with value1 and value2
    vals.push_back(
        kagome::scale::EncodeOpaqueValue{value_data2_encoded.asVector()});
    vals_encoded = Buffer(kagome::scale::encode(vals).value());
    EXPECT_CALL(*trie_batch_, put_rvalueHack(key_data, vals_encoded))
        .WillOnce(Return(outcome::success()));

    storage_extension_->ext_storage_append_version_1(key.combine(),
                                                     value2.combine());
  }
}

TEST_F(StorageExtensionTest, ExtStorageAppendTestCompactLenChanged) {
  // @given key and two values
  PtrSize key(43, 43);
  Buffer key_data(key.size, 'k');

  Buffer value_data1(42, '1');
  Buffer value_data1_encoded{kagome::scale::encode(value_data1).value()};
  PtrSize value1(42, value_data1_encoded.size());

  Buffer value_data2(43, '2');
  Buffer value_data2_encoded{kagome::scale::encode(value_data2).value()};
  PtrSize value2(45, value_data2_encoded.size());

  // @given wasm memory that can provide these key and values
  EXPECT_CALL(*memory_, loadN(key.ptr, key.size))
      .WillRepeatedly(Return(key_data));
  EXPECT_CALL(*memory_, loadN(value2.ptr, value2.size))
      .WillOnce(Return(value_data2_encoded));

  // @when vals contains (2^6 - 1) elements (high limit for one-byte compact
  // integers)
  std::vector<kagome::scale::EncodeOpaqueValue> vals(
      kagome::scale::compact::EncodingCategoryLimits::kMinUint16 - 1,
      kagome::scale::EncodeOpaqueValue{value_data1_encoded.asVector()});
  Buffer vals_encoded = Buffer(kagome::scale::encode(vals).value());

  {
    // @when encoded vals is stored by given key
    EXPECT_CALL(*trie_batch_, get(key_data)).WillOnce(Return(vals_encoded));

    // @when storage is inserted by one more value by the same key
    vals.push_back(
        kagome::scale::EncodeOpaqueValue{value_data2_encoded.asVector()});
    vals_encoded = Buffer(kagome::scale::encode(vals).value());

    // @then everything fine: storage is inserted with vals with new value
    EXPECT_CALL(*trie_batch_, put_rvalueHack(key_data, vals_encoded))
        .WillOnce(Return(outcome::success()));

    storage_extension_->ext_storage_append_version_1(key.combine(),
                                                     value2.combine());
  }
}

/**
 * @given a set of values, which ordered trie hash we want to calculate from
 * wasm
 * @when calling an extension method ext_blake2_256_enumerated_trie_root
 * @then the method reads the data from wasm memory properly and stores the
 * result in the wasm memory
 */
TEST_P(BuffersParametrizedTest, Blake2_256_EnumeratedTrieRoot) {
  auto &[values, hash_array] = GetParam();
  auto values_enc = kagome::scale::encode(values).value();

  using testing::_;
  PtrSize values_span {42, static_cast<WasmSize>(values_enc.size())};

  EXPECT_CALL(*memory_, loadN(values_span.ptr, values_span.size))
      .WillOnce(Return(Buffer {values_enc}));
  WasmPointer result = 1984;
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(hash_array)))
      .WillOnce(Return(result));

  storage_extension_->ext_trie_blake2_256_ordered_root_version_1(
      values_span.combine());
}

/**
 * @given a set of values, which ordered trie hash we want to calculate from
 * wasm
 * @when calling an extension method ext_blake2_256_ordered_trie_root
 * @then the method reads the data from wasm memory properly and stores the
 * result in the wasm memory
 */
TEST_P(BuffersParametrizedTest, Blake2_256_OrderedTrieRootV1) {
  auto &[values, hash_array] = GetParam();

  using testing::_;
  WasmPointer values_ptr = 1;
  WasmSize values_size = 2;
  WasmSpan values_data = PtrSize(values_ptr, values_size).combine();
  WasmPointer result = 1984;

  Buffer buffer{kagome::scale::encode(values).value()};

  EXPECT_CALL(*memory_, loadN(values_ptr, values_size))
      .WillOnce(Return(buffer));

  EXPECT_CALL(*memory_, storeBuffer(gsl::span<const uint8_t>(hash_array)))
      .WillOnce(Return(result));

  ASSERT_EQ(result,
            storage_extension_->ext_trie_blake2_256_ordered_root_version_1(
                values_data));
}

INSTANTIATE_TEST_CASE_P(
    Instance,
    BuffersParametrizedTest,
    testing::Values(
        // test from substrate:
        // https://github.com/paritytech/substrate/blob/f311d14f6fb76161950f0eca0b3f71a353824d46/core/executor/src/wasm_executor.rs#L1769
        EnumeratedTrieRootTestCase{
            std::list<Buffer>{"zero"_buf, "one"_buf, "two"_buf},
            "9243f4bb6fa633dce97247652479ed7e2e2995a5ea641fd9d1e1a046f7601da6"_hex2buf},
        // empty list case, hash also obtained from substrate
        EnumeratedTrieRootTestCase{
            std::list<Buffer>{},
            "03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314"_hex2buf}));

/**
 * @given key_pointer, key_size, value_ptr, value_size
 * @when ext_storage_get_version_1 is invoked on given key
 * @then corresponding value will be returned
 */
TEST_F(StorageExtensionTest, StorageGetV1Test) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');

  WasmPointer value_pointer = 42;
  WasmSize value_size = 41;

  WasmSpan value_span = PtrSize(value_pointer, value_size).combine();
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();

  Buffer value(8, 'v');
  auto encoded_opt_value =
      kagome::scale::encode<boost::optional<Buffer>>(value).value();

  // expect key and value were loaded
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*memory_,
              storeBuffer(gsl::span<const uint8_t>(encoded_opt_value)))
      .WillOnce(Return(value_span));

  // expect key-value pair was put to db
  EXPECT_CALL(*trie_batch_, get(key)).WillOnce(Return(value));

  ASSERT_EQ(value_span,
            storage_extension_->ext_storage_get_version_1(key_span));
}

/**
 * @given key_pointer and key_size
 * @when ext_storage_clear_version_1 is invoked on StorageExtension with given
 * key
 * @then key is loaded from the memory @and del is invoked on storage
 */
TEST_P(OutcomeParameterizedTest, ExtStorageClearV1Test) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();

  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  // to ensure that it works when remove() returns success or failure
  EXPECT_CALL(*trie_batch_, remove(key)).WillOnce(Return(GetParam()));

  storage_extension_->ext_storage_clear_version_1(key_span);
}

/**
 * @given key pointer and key size
 * @when ext_storage_exists_version_1 is invoked on StorageExtension with given
 * key
 * @then result is the same as result of contains on given key
 */
TEST_F(StorageExtensionTest, ExtStorageExistsV1Test) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');
  WasmSpan key_span = PtrSize(key_pointer, key_size).combine();

  /// result of contains method on db
  WasmSize contains = 1;

  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*trie_batch_, contains(key)).WillOnce(Return(contains));

  ASSERT_EQ(contains,
            storage_extension_->ext_storage_exists_version_1(key_span));
}

/**
 * @given prefix_pointer with prefix_length
 * @when ext_clear_prefix is invoked on StorageExtension with given prefix
 * @then prefix is loaded from the memory @and clearPrefix is invoked on storage
 */
TEST_F(StorageExtensionTest, ExtStorageClearPrefixV1Test) {
  WasmPointer prefix_pointer = 42;
  WasmSize prefix_size = 42;
  Buffer prefix(8, 'p');
  WasmSpan prefix_span = PtrSize(prefix_pointer, prefix_size).combine();

  EXPECT_CALL(*memory_, loadN(prefix_pointer, prefix_size))
      .WillOnce(Return(prefix));
  EXPECT_CALL(*trie_batch_, clearPrefix(prefix))
      .Times(1)
      .WillOnce(Return(outcome::success()));

  storage_extension_->ext_storage_clear_prefix_version_1(prefix_span);
}

/**
 * @given a set of values, which ordered trie hash we want to calculate from
 * wasm
 * @when calling an extension method ext_blake2_256_ordered_trie_root
 * @then the method reads the data from wasm memory properly and stores the
 * result in the wasm memory
 */
TEST_F(StorageExtensionTest, Blake2_256_TrieRootV1) {
  std::vector<std::pair<Buffer, Buffer>> dict = {
      {"a"_buf, "one"_buf}, {"b"_buf, "two"_buf}, {"c"_buf, "three"_buf}};
  auto hash_array =
      "eaa57e0e1a41d5a49db5954f95140a4e7c9a4373f7d29c0d667c9978ab4dadcb"_hex2buf;

  WasmPointer values_ptr = 1;
  WasmSize values_size = 2;
  WasmSpan dict_data = PtrSize(values_ptr, values_size).combine();
  WasmPointer result = 1984;

  Buffer buffer{kagome::scale::encode(dict).value()};

  EXPECT_CALL(*memory_, loadN(values_ptr, values_size))
      .WillOnce(Return(buffer));

  EXPECT_CALL(*memory_, storeBuffer(gsl::span<const uint8_t>(hash_array)))
      .WillOnce(Return(result));

  ASSERT_EQ(result,
            storage_extension_->ext_trie_blake2_256_root_version_1(dict_data));
}

/**
 * @given no :changes_trie key (which sets the changes trie config) in storage
 * @when calling ext_storage_changes_root_version_1
 * @then none is returned, changes trie is empty
 */
TEST_F(StorageExtensionTest, ChangesRootEmpty) {
  auto parent_hash = "123456"_hash256;
  Buffer parent_hash_buf{gsl::span(parent_hash.data(), parent_hash.size())};
  PtrSize parent_root_ptr{1, Hash256::size()};
  EXPECT_CALL(*memory_, loadN(parent_root_ptr.ptr, Hash256::size()))
      .WillOnce(Return(parent_hash_buf));

  EXPECT_CALL(*trie_batch_, get(kagome::common::Buffer{}.put(":changes_trie")))
      .WillOnce(Return(kagome::storage::trie::TrieError::NO_VALUE));

  WasmPointer result = 1984;
  Buffer none_bytes{0};
  EXPECT_CALL(*memory_, storeBuffer(gsl::span<const uint8_t>(none_bytes)))
      .WillOnce(Return(result));

  auto res = storage_extension_->ext_storage_changes_root_version_1(
      parent_root_ptr.combine());
  ASSERT_EQ(res, result);
}

MATCHER_P(configsAreEqual, n, "") {
  return (arg.digest_interval == n.digest_interval)
         and (arg.digest_levels == n.digest_levels);
}

/**
 * @given existing :changes_trie key (which sets the changes trie config) in
 * storage and there are accumulated changes
 * @when calling ext_storage_changes_root_version_1
 * @then a valid trie root is returned
 */
TEST_F(StorageExtensionTest, ChangesRootNotEmpty) {
  auto parent_hash = "123456"_hash256;
  Buffer parent_hash_buf{gsl::span(parent_hash.data(), parent_hash.size())};
  PtrSize parent_root_ptr{1, Hash256::size()};
  EXPECT_CALL(*memory_, loadN(parent_root_ptr.ptr, Hash256::size()))
      .WillOnce(Return(parent_hash_buf));

  kagome::storage::changes_trie::ChangesTrieConfig config{.digest_interval = 0,
                                                          .digest_levels = 0};
  EXPECT_CALL(*trie_batch_, get(kagome::common::Buffer{}.put(":changes_trie")))
      .WillOnce(Return(Buffer(kagome::scale::encode(config).value())));

  auto trie_hash = "deadbeef"_hash256;
  auto enc_trie_hash =
      kagome::scale::encode(
          boost::make_optional((gsl::span(trie_hash.data(), Hash256::size()))))
          .value();
  EXPECT_CALL(*changes_tracker_,
              constructChangesTrie(parent_hash, configsAreEqual(config)))
      .WillOnce(Return(trie_hash));

  WasmPointer result = 1984;
  Buffer none_bytes{0};
  EXPECT_CALL(*memory_, storeBuffer(gsl::span<const uint8_t>(enc_trie_hash)))
      .WillOnce(Return(result));

  auto res = storage_extension_->ext_storage_changes_root_version_1(
      parent_root_ptr.combine());
  ASSERT_EQ(res, result);
}
