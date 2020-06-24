/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/storage_extension.hpp"

#include <gtest/gtest.h>

#include "core/runtime/mock_memory.hpp"
#include "mock/core/runtime/trie_storage_provider_mock.hpp"
#include "mock/core/storage/changes_trie/changes_tracker_mock.hpp"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::extensions::StorageExtension;
using kagome::runtime::MockMemory;
using kagome::runtime::TrieStorageProviderMock;
using kagome::runtime::WasmPointer;
using kagome::runtime::WasmSize;
using kagome::storage::changes_trie::ChangesTrackerMock;
using kagome::storage::trie::EphemeralTrieBatchMock;
using kagome::storage::trie::PersistentTrieBatchMock;

using ::testing::_;
using ::testing::Return;

class StorageExtensionTest : public ::testing::Test {
 public:
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
    memory_ = std::make_shared<MockMemory>();
    changes_tracker_ = std::make_shared<ChangesTrackerMock>();
    storage_extension_ = std::make_shared<StorageExtension>(
        storage_provider_, memory_, changes_tracker_);
  }

 protected:
  std::shared_ptr<PersistentTrieBatchMock> trie_batch_;
  std::shared_ptr<TrieStorageProviderMock> storage_provider_;
  std::shared_ptr<MockMemory> memory_;
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

  storage_extension_->ext_clear_prefix(prefix_pointer, prefix_size);
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

  storage_extension_->ext_clear_storage(key_pointer, key_size);
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
            storage_extension_->ext_exists_storage(key_pointer, key_size));
}

/**
 * @given key_pointer, key_size of non-existing key and pointer where length
 * will be stored
 * @when ext_get_allocated_storage is invoked on given key and provided
 * length
 * @then length ptr is pointing to the u32::max() and function
 * returns 0
 */
TEST_F(StorageExtensionTest, GetAllocatedStorageKeyNotExistsTest) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');

  WasmPointer len_ptr = 123;

  /// res with any error, to indicate that get has been failed
  outcome::result<Buffer> get_res = outcome::failure(std::error_code());

  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*trie_batch_, get(key)).WillOnce(Return(get_res));

  EXPECT_CALL(*memory_, store32(len_ptr, kU32Max));
  ASSERT_EQ(0,
            storage_extension_->ext_get_allocated_storage(
                key_pointer, key_size, len_ptr));
}

/**
 * @given key_pointer, key_size of existing key and pointer where length
 * will be stored
 * @when ext_get_allocated_storage is invoked on given key and provided
 * length
 * @then length ptr is pointing to the value's length and result of the function
 * contains the pointer to the memory allocated for the value returns 0
 */
TEST_F(StorageExtensionTest, GetAllocatedStorageKeyExistTest) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');

  WasmPointer len_ptr = 123;

  /// res with value
  WasmSize value_length = 12;
  outcome::result<Buffer> get_res = Buffer(value_length, 'v');

  // expect key was loaded
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*trie_batch_, get(key)).WillOnce(Return(get_res));

  // value length is stored at len ptr as expected
  EXPECT_CALL(*memory_, store32(len_ptr, value_length)).Times(1);

  WasmPointer allocated_value_ptr = 321;
  // memory for the value is expected to be allocated
  EXPECT_CALL(*memory_, allocate(value_length))
      .WillOnce(Return(allocated_value_ptr));
  // value is stored in allocated memory
  EXPECT_CALL(*memory_,
              storeBuffer(allocated_value_ptr,
                          gsl::span<const uint8_t>(get_res.value())));

  // ptr for the allocated value is returned
  ASSERT_EQ(allocated_value_ptr,
            storage_extension_->ext_get_allocated_storage(
                key_pointer, key_size, len_ptr));
}

/**
 * @given key_pointer, key_size of existing key, value_ptr where value will be
 * stored with given offset and length
 * @when ext_get_storage_into is invoked on given key, value_ptr, offset and
 * length
 * @then then value associated with the key is stored on value_ptr with given
 * offset and length @and ext_get_storage_into returns the size of the value
 * written
 */
TEST_F(StorageExtensionTest, GetStorageIntoKeyExistsTest) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');

  auto value = "abcdef"_buf;
  WasmPointer value_ptr = 123;
  WasmSize value_length = 2;
  WasmSize value_offset = 3;
  Buffer partial_value(std::vector<uint8_t>{
      value.toVector().begin() + value_offset,
      value.toVector().begin() + value_offset + value_length});

  // expect key was loaded
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));
  EXPECT_CALL(*trie_batch_, get(key)).WillOnce(Return(value));

  // only partial value (which is the slice value[offset, offset+length]) should
  // be stored at value_ptr
  EXPECT_CALL(*memory_,
              storeBuffer(value_ptr, gsl::span<const uint8_t>(partial_value)));

  // ext_get_storage_into should return the length of stored partial value
  ASSERT_EQ(partial_value.size(),
            storage_extension_->ext_get_storage_into(
                key_pointer, key_size, value_ptr, value_length, value_offset));
}

/**
 * @given key_pointer, key_size of non-existing key, and arbitrary value_ptr,
 * value_offset and value_length
 * @when ext_get_storage_into is invoked on given key, value_ptr, offset and
 * length
 * @then ext_get_storage_into returns u32::max()
 */
TEST_F(StorageExtensionTest, GetStorageIntoKeyNotExistsTest) {
  WasmPointer key_pointer = 43;
  WasmSize key_size = 43;
  Buffer key(8, 'k');

  WasmPointer value_ptr = 123;
  WasmSize value_length = 2;
  WasmSize value_offset = 3;

  // expect key was loaded
  EXPECT_CALL(*memory_, loadN(key_pointer, key_size)).WillOnce(Return(key));

  // get(key) will return error
  EXPECT_CALL(*trie_batch_, get(key))
      .WillOnce(Return(outcome::failure(std::error_code())));

  // ext_get_storage_into should return u32::max()
  ASSERT_EQ(kU32Max,
            storage_extension_->ext_get_storage_into(
                key_pointer, key_size, value_ptr, value_length, value_offset));
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

  storage_extension_->ext_set_storage(
      key_pointer, key_size, value_pointer, value_size);
}

INSTANTIATE_TEST_CASE_P(Instance,
                        OutcomeParameterizedTest,
                        ::testing::Values<outcome::result<void>>(
                            /// success case
                            outcome::success(),
                            /// failure with arbitrary error code
                            outcome::failure(std::error_code())),
                        // empty argument for the macro
);

/**
 * @given a set of values, which ordered trie hash we want to calculate from
 * wasm
 * @when calling an extension method ext_blake2_256_enumerated_trie_root
 * @then the method reads the data from wasm memory properly and stores the
 * result in the wasm memory
 */
TEST_P(BuffersParametrizedTest, Blake2_256_EnumeratedTrieRoot) {
  auto &[values, hash_array] = GetParam();

  using testing::_;
  WasmPointer values_ptr = 42;
  WasmPointer lens_ptr = 1337;
  uint32_t val_offset = 0;
  uint32_t len_offset = 0;
  for (auto &&v : values) {
    EXPECT_CALL(*memory_, load32u(lens_ptr + len_offset))
        .WillOnce(Return(v.size()));
    EXPECT_CALL(*memory_, loadN(values_ptr + val_offset, v.size()))
        .WillOnce(Return(v));
    val_offset += v.size();
    len_offset += 4;
  }
  WasmPointer result = 1984;
  EXPECT_CALL(*memory_,
              storeBuffer(result, gsl::span<const uint8_t>(hash_array)))
      .Times(1);

  storage_extension_->ext_blake2_256_enumerated_trie_root(
      values_ptr, lens_ptr, values.size(), result);
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
