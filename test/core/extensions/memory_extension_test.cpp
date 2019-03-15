/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/memory_extension.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "extensions/impl/memory_extension.hpp"

using namespace kagome::extensions;

using kagome::runtime::Memory;

using ::testing::Return;

class MockMemory : public Memory {
 public:
  MOCK_METHOD1(resize, void(uint32_t));
  MOCK_METHOD1(allocate, std::optional<AddressType>(uint32_t));
  MOCK_METHOD1(deallocate, std::optional<SizeType>(AddressType));

  MOCK_METHOD1(load8s, int8_t(AddressType));
  MOCK_METHOD1(load8u, uint8_t(AddressType));
  MOCK_METHOD1(load16s, int16_t(AddressType));
  MOCK_METHOD1(load16u, uint16_t(AddressType));
  MOCK_METHOD1(load32s, int32_t(AddressType));
  MOCK_METHOD1(load32u, uint32_t(AddressType));
  MOCK_METHOD1(load64s, int64_t(AddressType));
  MOCK_METHOD1(load64u, uint64_t(AddressType));
  MOCK_METHOD1(load128, std::array<uint8_t, 16>(AddressType));

  MOCK_METHOD2(store8, void(AddressType, int8_t));
  MOCK_METHOD2(store16, void(AddressType, int16_t));
  MOCK_METHOD2(store32, void(AddressType, int32_t));
  MOCK_METHOD2(store64, void(AddressType, int64_t));
  MOCK_METHOD2(store128, void(AddressType, const std::array<uint8_t, 16> &));
};

class MemoryExtensionsTest : public ::testing::Test {
 public:
  void SetUp() override {
    memory_ = std::make_shared<MockMemory>();
    memory_extension_ = MemoryExtension(memory_);
  }

 protected:
  std::shared_ptr<MockMemory> memory_;
  MemoryExtension memory_extension_;
};

/**
 * @given MemoryExtension initialized with the memory
 * @when ext_malloc is invoked on MemoryExtension
 * @then ext_malloc invokes allocate method from Memory and returns its result
 */
TEST_F(MemoryExtensionsTest, MallocIsCalled) {
  const uint32_t allocated_size = 10;
  // expected address is 0 because it is the first memory chunk
  const Memory::AddressType expected_address = 0;
  EXPECT_CALL(*memory_, allocate(allocated_size))
      .WillOnce(Return(expected_address));

  auto ptr = memory_extension_.ext_malloc(allocated_size);
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
  std::optional<uint32_t> deallocate_result{42};
  EXPECT_CALL(*memory_, deallocate(ptr)).WillOnce(Return(deallocate_result));

  memory_extension_.ext_free(ptr);
}
