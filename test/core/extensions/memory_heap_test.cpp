/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MEMORY_HEAP_TEST_HPP
#define KAGOME_MEMORY_HEAP_TEST_HPP

#include <gtest/gtest.h>

#include "extensions/impl/memory.hpp"

using namespace kagome::extensions;

class MemoryTest : public ::testing::Test {
 public:
  const size_t memory_size_ = 10;
  Memory memory_{memory_size_};
};

TEST_F(MemoryTest, Return0WhenSize0) {
  ASSERT_EQ(memory_.allocate(0), 0);
}

TEST_F(MemoryTest, TryAllocatedTooBigMemory) {
  const auto allocated_memory = memory_size_ + 1;
  ASSERT_FALSE(memory_.allocate(allocated_memory).has_value());
}

/**
 * @given memory with already allocated memory of size1
 * @when allocate memory with size2
 * @then the pointer pointing to the end of the first memory chunk is returned
 */
TEST_F(MemoryTest, ReturnOffsetWhenAllocated) {
  const size_t size1 = 3;
  const size_t size2 = 4;

  // allocate memory of size 1
  auto opt_ptr1 = memory_.allocate(size1);
  ASSERT_TRUE(opt_ptr1.has_value());
  // first memory chunk is always allocated at 0
  ASSERT_EQ(opt_ptr1.value(), 0);

  // allocated second memory chunk
  auto opt_ptr2 = memory_.allocate(size2);
  ASSERT_TRUE(opt_ptr2.has_value());
  // second memory chunk is placed right after the first one
  ASSERT_EQ(opt_ptr2.value(), size1);
}

/**
 * @given memory with allocated memory chunk
 * @when this memory is deallocated
 * @then the size of this memory chunk is returned
 */
TEST_F(MemoryTest, DeallocateExisingMemoryChunk) {
  const size_t size1 = 3;

  auto ptr1 = memory_.allocate(size1).value();

  auto opt_deallocated_size = memory_.deallocate(ptr1);
  ASSERT_TRUE(opt_deallocated_size.has_value());
  ASSERT_EQ(opt_deallocated_size.value(), size1);
}

/**
 * @given memory with memory chunk allocated at the beginning
 * @when try to deallocate is invoked with ptr that does not point to any memory
 * chunk
 * @then deallocate returns none
 */
TEST_F(MemoryTest, DeallocateNonexistingMemoryChunk) {
  const size_t size1 = 3;

  auto ptr1 = memory_.allocate(size1).value();

  auto ptr_to_nonexisting_chunk = 2;
  auto opt_deallocated_size = memory_.deallocate(ptr_to_nonexisting_chunk);
  ASSERT_FALSE(opt_deallocated_size.has_value());
}

/**
 * @given memory with two memory chunk filling entire memory
 * @when first memory chunk of size size1 is deallocated @and new memory chunk
 * of the same size is trying to be allocated on that memory
 * @then it is allocated on the place of the first memory chunk
 */
TEST_F(MemoryTest, AllocateAfterDeallocate) {
  // two memory sizes totalling to the total memory size
  const size_t size1 = 3;
  const size_t size2 = 7;

  // allocate two memory chunks with total size equal to the memory size
  auto ptr1 = memory_.allocate(size1).value();
  memory_.allocate(size2).value();

  // deallocate first memory chunk
  memory_.deallocate(ptr1);

  // allocate new memory chunk
  auto ptr1_1 = memory_.allocate(size1).value();
  // expected that it will be allocated on the same place as the first memory
  // chunk that was deallocated
  ASSERT_EQ(ptr1, ptr1_1);
}

TEST_F(MemoryTest, AllocateTooBigMemoryAfterDeallocate) {
  // two memory sizes totalling to the total memory size
  const size_t size1 = 3;
  const size_t size2 = 7;

  // allocate two memory chunks with total size equal to the memory size
  auto ptr1 = memory_.allocate(size1).value();
  memory_.allocate(size2).value();

  // deallocate first memory chunk
  memory_.deallocate(ptr1);

  // allocate new memory chunk with bigger size than the space left in the memory
  auto opt_ptr = memory_.allocate(size1+1);
  ASSERT_FALSE(opt_ptr.has_value()) << opt_ptr.value();
}

#endif  // KAGOME_MEMORY_HEAP_TEST_HPP
