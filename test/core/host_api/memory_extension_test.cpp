/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/memory_extension.hpp"

#include <gtest/gtest.h>

#include "mock/core/runtime/memory_mock.hpp"
#include "mock/core/runtime/memory_provider_mock.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome::host_api;

using kagome::common::Buffer;
using kagome::runtime::Memory;
using kagome::runtime::MemoryMock;
using kagome::runtime::MemoryProviderMock;
using kagome::runtime::WasmPointer;
using testing::Return;

using ::testing::Return;

class MemoryExtensionsTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    memory_provider_ = std::make_shared<MemoryProviderMock>();
    memory_ = std::make_shared<MemoryMock>();
    EXPECT_CALL(*memory_provider_, getCurrentMemory())
        .WillRepeatedly(Return(boost::optional<std::shared_ptr<Memory>>(memory_)));
    memory_extension_ = std::make_shared<MemoryExtension>(memory_provider_);
  }

 protected:
  std::shared_ptr<MemoryProviderMock> memory_provider_;
  std::shared_ptr<MemoryMock> memory_;
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

  auto ptr = memory_extension_->ext_allocator_malloc_version_1(allocated_size);
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

  memory_extension_->ext_allocator_free_version_1(ptr);
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
