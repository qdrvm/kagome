/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/memory_extension.hpp"

#include <gtest/gtest.h>

#include "mock/core/runtime/wasm_memory_mock.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome::host_api;

using kagome::common::Buffer;
using kagome::runtime::WasmMemory;
using kagome::runtime::WasmMemoryMock;
using kagome::runtime::WasmPointer;

using ::testing::Return;

class MemoryExtensionsTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    memory_ = std::make_shared<WasmMemoryMock>();
    memory_extension_ = std::make_shared<MemoryExtension>(memory_);
  }

 protected:
  std::shared_ptr<WasmMemoryMock> memory_;
  std::shared_ptr<MemoryExtension> memory_extension_;
};

/**
 * @given MemoryExtension initialized with the memory
 * @when ext_malloc is invoked on MemoryExtension
 * @then ext_malloc invokes allocate method from Memory and returns its result
 */
TEST_F(MemoryExtensionsTest, MallocIsCalled) {
  const uint32_t allocated_size = 10;
  // expected address is 0 because it is the first memory chunk
  const WasmPointer expected_address = 0;
  EXPECT_CALL(*memory_, allocate(allocated_size))
      .WillOnce(Return(expected_address));

  auto ptr = memory_extension_->ext_malloc(allocated_size);
  ASSERT_EQ(ptr, expected_address);
}

/**
 * @given MemoryExtension initialized with the memory
 * @when ext_free is invoked on it
 * @then deallocate is invoked on Memory object
 */
TEST_F(MemoryExtensionsTest, FreeIsCalled) {
  int32_t ptr = 0;
  // result of deallocate method, could be basically anything
  boost::optional<uint32_t> deallocate_result{42};
  EXPECT_CALL(*memory_, deallocate(ptr)).WillOnce(Return(deallocate_result));

  memory_extension_->ext_free(ptr);
}

/**
 * @given MemoryExtension initialized with the memory
 * @when ext_malloc is invoked on MemoryExtension
 * @then ext_malloc invokes allocate method from Memory and returns its result
 */
TEST_F(MemoryExtensionsTest, MallocV1IsCalled) {
  const uint32_t allocated_size = 10;
  // expected address is 0 because it is the first memory chunk
  const WasmPointer expected_address = 0;
  EXPECT_CALL(*memory_, allocate(allocated_size))
      .WillOnce(Return(expected_address));

  auto ptr = memory_extension_->ext_allocator_malloc_version_1(allocated_size);
  ASSERT_EQ(ptr, expected_address);
}

/**
 * @given MemoryExtension initialized with the memory
 * @when ext_free is invoked on it
 * @then deallocate is invoked on Memory object
 */
TEST_F(MemoryExtensionsTest, FreeV1IsCalled) {
  int32_t ptr = 0;
  // result of deallocate method, could be basically anything
  boost::optional<uint32_t> deallocate_result{42};
  EXPECT_CALL(*memory_, deallocate(ptr)).WillOnce(Return(deallocate_result));

  memory_extension_->ext_allocator_free_version_1(ptr);
}
