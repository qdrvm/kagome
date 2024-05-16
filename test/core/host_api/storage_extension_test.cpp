/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/storage_extension.hpp"

#include <optional>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <scale/scale.hpp>

#include "crypto/hasher/hasher_impl.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/trie/polkadot_trie_cursor_mock.h"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "runtime/ptr_size.hpp"
#include "scale/encode_append.hpp"
#include "scale/kagome_scale.hpp"
#include "storage/predefined_keys.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/outcome/dummy_error.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/runtime/memory.hpp"
#include "testutil/scale_test_comparator.hpp"

using kagome::ClearPrefixLimit;
using kagome::KillStorageResult;
using kagome::common::Buffer;
using kagome::common::BufferView;
using kagome::common::Hash256;
using kagome::host_api::StorageExtension;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::PtrSize;
using kagome::runtime::TestMemory;
using kagome::runtime::TrieStorageProviderMock;
using kagome::runtime::WasmOffset;
using kagome::runtime::WasmPointer;
using kagome::runtime::WasmSize;
using kagome::runtime::WasmSpan;
using kagome::storage::trie::PolkadotCodec;
using kagome::storage::trie::PolkadotTrieCursorMock;
using kagome::storage::trie::RootHash;
using kagome::storage::trie::TrieBatchMock;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

constexpr std::optional<BufferView> no_child;

class StorageExtensionTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    trie_batch_ = std::make_shared<TrieBatchMock>();
    storage_provider_ = std::make_shared<TrieStorageProviderMock>();
    EXPECT_CALL(*storage_provider_, getCurrentBatch())
        .WillRepeatedly(Return(trie_batch_));
    memory_provider_ = std::make_shared<MemoryProviderMock>();
    EXPECT_CALL(*memory_provider_, getCurrentMemory())
        .WillRepeatedly(Return(std::ref(memory_.memory)));
    storage_extension_ = std::make_shared<StorageExtension>(
        storage_provider_,
        memory_provider_,
        std::make_shared<kagome::crypto::HasherImpl>());
  }

 protected:
  std::shared_ptr<TrieBatchMock> trie_batch_;
  std::shared_ptr<TrieStorageProviderMock> storage_provider_;
  TestMemory memory_;
  std::shared_ptr<MemoryProviderMock> memory_provider_;
  std::shared_ptr<StorageExtension> storage_extension_;
  PolkadotCodec codec_;

  static constexpr uint32_t kU32Max = std::numeric_limits<uint32_t>::max();
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
 * @given key_pointer and key_size
 * @when ext_clear_storage is invoked on StorageExtension with given key
 * @then key is loaded from the memory @and del is invoked on storage
 */
TEST_P(OutcomeParameterizedTest, ClearStorageTest) {
  Buffer key(8, 'k');

  EXPECT_CALL(*trie_batch_, remove(BufferView{key}))
      .WillOnce(Return(GetParam()));

  storage_extension_->ext_storage_clear_version_1(memory_[key]);
}

/**
 * @given key pointer and key size
 * @when ext_exists_storage is invoked on StorageExtension with given key
 * @then result is the same as result of contains on given key
 */
TEST_F(StorageExtensionTest, ExistsStorageTest) {
  Buffer key(8, 'k');

  /// result of contains method on db
  WasmSize contains = 1;

  EXPECT_CALL(*trie_batch_, contains(key.view())).WillOnce(Return(contains));

  ASSERT_EQ(contains,
            storage_extension_->ext_storage_exists_version_1(memory_[key]));
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
  Buffer key(8, 'k');
  Buffer expected_next_key(9, 'k');

  EXPECT_CALL(*trie_batch_, trieCursor())
      .WillOnce(Invoke([&key, &expected_next_key]() {
        auto cursor = std::make_unique<PolkadotTrieCursorMock>();
        EXPECT_CALL(*cursor, seekUpperBound(key.view()))
            .WillOnce(Return(outcome::success()));
        EXPECT_CALL(*cursor, key()).WillOnce(Return(expected_next_key));
        return cursor;
      }));

  ASSERT_EQ(
      memory_.decode<std::optional<Buffer>>(
          storage_extension_->ext_storage_next_key_version_1(memory_[key])),
      expected_next_key);
}

/**
 * @given a trie key address in WASM memory, to which there is no key
 * lexicographically greater
 * @when using ext_storage_next_key_version_1 to obtain the next key
 * @then an address of none value is returned
 */
TEST_F(StorageExtensionTest, NextKeyLastKey) {
  Buffer key(8, 'k');

  EXPECT_CALL(*trie_batch_, trieCursor()).WillOnce(Invoke([&key]() {
    auto cursor = std::make_unique<PolkadotTrieCursorMock>();
    EXPECT_CALL(*cursor, seekUpperBound(key.view()))
        .WillOnce(Return(outcome::success()));
    EXPECT_CALL(*cursor, key()).WillOnce(Return(std::nullopt));
    return cursor;
  }));

  ASSERT_EQ(
      memory_.decode<std::optional<Buffer>>(
          storage_extension_->ext_storage_next_key_version_1(memory_[key])),
      std::nullopt);
}

/**
 * @given a trie key address in WASM memory, which is not present in the storage
 * @when using ext_storage_next_key_version_1 to obtain the next key
 * @then an invalid address is returned
 */
TEST_F(StorageExtensionTest, NextKeyEmptyTrie) {
  Buffer key(8, 'k');

  EXPECT_CALL(*trie_batch_, trieCursor()).WillOnce(Invoke([&key]() {
    auto cursor = std::make_unique<PolkadotTrieCursorMock>();
    EXPECT_CALL(*cursor, seekUpperBound(key.view()))
        .WillOnce(Return(outcome::success()));
    EXPECT_CALL(*cursor, key()).WillOnce(Return(std::nullopt));
    return cursor;
  }));

  ASSERT_EQ(
      memory_.decode<std::optional<Buffer>>(
          storage_extension_->ext_storage_next_key_version_1(memory_[key])),
      std::nullopt);
}

/**
 * @given key_pointer, key_size, value_ptr, value_size
 * @when ext_set_storage is invoked on given key and value
 * @then provided key and value are put to db
 */
TEST_P(OutcomeParameterizedTest, SetStorageTest) {
  Buffer key(8, 'k');
  Buffer value(8, 'v');

  // expect key-value pair was put to db
  EXPECT_CALL(*trie_batch_, put(key.view(), value))
      .WillOnce(Return(GetParam()));

  storage_extension_->ext_storage_set_version_1(memory_[key], memory_[value]);
}

/**
 * @given key, value, offset
 * @when ext_storage_read_version_1 is invoked on given key and value
 * @then data read from db with given key
 */
TEST_P(OutcomeParameterizedTest, StorageReadTest) {
  Buffer key(8, 'k');
  Buffer value(8, 'v');
  WasmOffset offset = 4;
  std::optional<uint32_t> result{value.size() - offset};

  // expect key loaded, then data stored
  EXPECT_CALL(*storage_provider_, getCurrentBatch())
      .WillOnce(Return(trie_batch_));
  EXPECT_CALL(*trie_batch_, tryGetMock(key.view())).WillOnce(Return(value));

  auto value_span = memory_.allocate2(2);
  ASSERT_EQ(memory_.decode<decltype(result)>(
                storage_extension_->ext_storage_read_version_1(
                    memory_[key], value_span.combine(), offset)),
            result);
  auto n = std::min<size_t>(value_span.size, value.size());
  ASSERT_EQ(memory_.memory.view(value_span.ptr, n).value(),
            SpanAdl{value.view(offset, n)});
}

INSTANTIATE_TEST_SUITE_P(Instance,
                         OutcomeParameterizedTest,
                         ::testing::Values<outcome::result<void>>(
                             /// success case
                             outcome::success(),
                             /// failure with arbitrary error code
                             outcome::failure(testutil::DummyError::ERROR))
                         // empty argument for the macro
);

TEST_F(StorageExtensionTest, ExtStorageAppendTest) {
  // @given key and two values
  Buffer key(8, 'k');
  Buffer value1(42, '1');
  Buffer value2(43, '2');

  std::vector<scale::EncodeOpaqueValue> vals;
  Buffer vals_encoded;
  {
    // @when there is no value by given key in trie
    EXPECT_CALL(*trie_batch_, tryGetMock(key.view()))
        .WillOnce(Return(std::nullopt));

    // @then storage is inserted by scale encoded vector containing
    // EncodeOpaqueValue with value1
    vals.push_back(scale::EncodeOpaqueValue{value1});
    vals_encoded = Buffer(scale::encode(vals).value());
    EXPECT_CALL(*trie_batch_, put(key.view(), vals_encoded))
        .WillOnce(Return(outcome::success()));

    storage_extension_->ext_storage_append_version_1(memory_[key],
                                                     memory_[value1]);
  }

  {
    // @when there is a value by given key (inserted above)
    EXPECT_CALL(*trie_batch_, tryGetMock(key.view()))
        .WillOnce(Return(vals_encoded));

    // @then storage is inserted by scale encoded vector containing two
    // EncodeOpaqueValues with value1 and value2
    vals.push_back(scale::EncodeOpaqueValue{value2});
    vals_encoded = Buffer(scale::encode(vals).value());
    EXPECT_CALL(*trie_batch_, put(key.view(), vals_encoded))
        .WillOnce(Return(outcome::success()));

    storage_extension_->ext_storage_append_version_1(memory_[key],
                                                     memory_[value2]);
  }
}

TEST_F(StorageExtensionTest, ExtStorageAppendTestCompactLenChanged) {
  // @given key and two values
  Buffer key(8, 'k');
  Buffer value1(1, '1');
  Buffer value2(43, '2');

  // @when vals contains (2^6 - 1) elements (high limit for one-byte compact
  // integers)
  std::vector<scale::EncodeOpaqueValue> vals(
      scale::compact::EncodingCategoryLimits::kMinUint16 - 1,
      scale::EncodeOpaqueValue{value1});
  Buffer vals_encoded = Buffer(scale::encode(vals).value());

  {
    // @when encoded vals is stored by given key
    EXPECT_CALL(*trie_batch_, tryGetMock(key.view()))
        .WillOnce(Return(vals_encoded));

    // @when storage is inserted by one more value by the same key
    vals.push_back(scale::EncodeOpaqueValue{value2});
    vals_encoded = Buffer(scale::encode(vals).value());

    // @then everything fine: storage is inserted with vals with new value
    EXPECT_CALL(*trie_batch_, put(key.view(), vals_encoded))
        .WillOnce(Return(outcome::success()));

    storage_extension_->ext_storage_append_version_1(memory_[key],
                                                     memory_[value2]);
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
  ASSERT_EQ(
      memory_.memory
          .view(storage_extension_->ext_trie_blake2_256_ordered_root_version_1(
                    memory_.encode(values)),
                hash_array.size())
          .value(),
      SpanAdl{hash_array});
}

INSTANTIATE_TEST_SUITE_P(
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
  Buffer key(8, 'k');
  Buffer value(8, 'v');

  // expect key-value pair was put to db
  EXPECT_CALL(*trie_batch_, tryGetMock(key.view())).WillOnce(Return(value));

  ASSERT_EQ(memory_.decode<std::optional<Buffer>>(
                storage_extension_->ext_storage_get_version_1(memory_[key])),
            value);
}

/**
 * @given key pointer and key size
 * @when ext_storage_exists_version_1 is invoked on StorageExtension with given
 * key
 * @then result is the same as result of contains on given key
 */
TEST_F(StorageExtensionTest, ExtStorageExistsV1Test) {
  Buffer key(8, 'k');

  /// result of contains method on db
  WasmSize contains = 1;

  EXPECT_CALL(*trie_batch_, contains(key.view())).WillOnce(Return(contains));

  ASSERT_EQ(contains,
            storage_extension_->ext_storage_exists_version_1(memory_[key]));
}

/**
 * @given prefix_pointer with prefix_length
 * @when ext_clear_prefix is invoked on StorageExtension with given prefix, up
 * to given limit
 * @then prefix is loaded from the memory @and clearPrefix is invoked on storage
 */
TEST_F(StorageExtensionTest, ExtStorageClearPrefixV1Test) {
  Buffer prefix(8, 'p');

  ClearPrefixLimit limit;
  EXPECT_CALL(*storage_provider_,
              clearPrefix(no_child, BufferView{prefix}, limit))
      .WillOnce(Return(KillStorageResult{}));

  storage_extension_->ext_storage_clear_prefix_version_1(prefix);
}

/**
 * @given prefix_pointer with prefix_length
 * @when ext_clear_prefix is invoked on StorageExtension with given prefix
 * @then prefix/limit is loaded from the memory @and clearPrefix is invoked on
 * storage with limit
 */
TEST_F(StorageExtensionTest, ExtStorageClearPrefixV2Test) {
  Buffer prefix(8, 'p');
  ClearPrefixLimit limit{22};

  KillStorageResult result{true, 22};
  EXPECT_CALL(*storage_provider_,
              clearPrefix(no_child, BufferView{prefix}, limit))
      .WillOnce(Return(result));

  ASSERT_EQ(
      storage_extension_->ext_storage_clear_prefix_version_2(prefix, limit),
      result);
}

/**
 * @given
 * @when invoke ext_storage_root_version_1
 * @then returns new root value
 */
TEST_F(StorageExtensionTest, RootTest) {
  // rest of ext_storage_root_version_1
  RootHash root_val = "123456"_hash256;
  EXPECT_CALL(*storage_provider_, commit(no_child, _))
      .WillOnce(Return(outcome::success(root_val)));

  ASSERT_EQ(storage_extension_->ext_storage_root_version_1(), root_val);
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
      "eaa57e0e1a41d5a49db5954f95140a4e7c9a4373f7d29c0d667c9978ab4dadcb"_unhex;
  ASSERT_EQ(memory_.memory
                .view(storage_extension_->ext_trie_blake2_256_root_version_1(
                          memory_.encode(dict)),
                      hash_array.size())
                .value(),
            SpanAdl{hash_array});
}
