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

#include "extensions/impl/memory_extension.hpp"

#include <gtest/gtest.h>

#include "core/runtime/mock_memory.hpp"

using namespace kagome::extensions;

using kagome::common::Buffer;
using kagome::runtime::MockMemory;
using kagome::runtime::SizeType;
using kagome::runtime::WasmMemory;
using kagome::runtime::WasmPointer;

using ::testing::Return;

class MemoryExtensionsTest : public ::testing::Test {
 public:
  void SetUp() override {
    memory_ = std::make_shared<MockMemory>();
    memory_extension_ = std::make_shared<MemoryExtension>(memory_);
  }

 protected:
  std::shared_ptr<MockMemory> memory_;
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
