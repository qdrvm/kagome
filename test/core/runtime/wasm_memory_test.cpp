/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "runtime/binaryen/wasm_memory_impl.hpp"

using kagome::runtime::binaryen::roundUpAlign;
using kagome::runtime::binaryen::WasmMemoryImpl;

class MemoryHeapTest : public ::testing::Test, public WasmMemoryImpl {
 public:
  MemoryHeapTest()
      : WasmMemoryImpl(getNewShellExternalInterface(), memory_size_) {}

 protected:
  const static uint32_t memory_size_ = 4096;  // one page size

  static wasm::ShellExternalInterface::Memory *getNewShellExternalInterface() {
    static std::unique_ptr<wasm::ShellExternalInterface::Memory> interface_;
    interface_ = std::make_unique<wasm::ShellExternalInterface::Memory>();
    return interface_.get();
  }
};

/**
 * @given memory of arbitrary size
 * @when trying to allocate memory of size 0
 * @then zero pointer is returned
 */
TEST_F(MemoryHeapTest, Return0WhenSize0) {
  ASSERT_EQ(allocate(0), 0);
}

/**
 * @given memory of size memory_size_
 * @when trying to allocate memory of size bigger than memory_size_ but less
 * than max memory size
 * @then -1 is not returned by allocate method indicating that memory was
 * allocated
 */
TEST_F(MemoryHeapTest, AllocatedMoreThanMemorySize) {
  const auto allocated_memory = memory_size_ + 1;
  ASSERT_NE(allocate(allocated_memory), -1);
}

/**
 * @given memory of size memory_size_ that is fully allocated
 * @when trying to allocate memory of size bigger than
 * (kMaxMemorySize-memory_size_)
 * @then -1 is not returned by allocate method indicating that memory was not
 * allocated
 */
TEST_F(MemoryHeapTest, AllocatedTooBigMemoryFailed) {
  // fully allocate memory
  auto ptr1 = allocate(memory_size_);
  // check that ptr1 is not -1, thus memory was allocated
  ASSERT_NE(ptr1, -1);

  // The memory size that can be allocated is within interval (0, kMaxMemorySize
  // - memory_size_]. Trying to allocate more
  auto big_memory_size = WasmMemoryImpl::kMaxMemorySize - memory_size_ + 1;
  ASSERT_EQ(allocate(big_memory_size), 0);
}

/**
 * @given memory with already allocated memory of size1
 * @when allocate memory with size2
 * @then the pointer pointing to the end of the first memory chunk is returned
 */
TEST_F(MemoryHeapTest, ReturnOffsetWhenAllocated) {
  const size_t size1 = 2049;
  const size_t size2 = 2045;

  // allocate memory of size 1
  auto ptr1 = allocate(size1);
  // first memory chunk is always allocated at min non-zero aligned address
  ASSERT_EQ(ptr1, roundUpAlign(1));

  // allocated second memory chunk
  auto ptr2 = allocate(size2);
  // second memory chunk is placed right after the first one (alligned by 4)
  ASSERT_EQ(ptr2, kagome::runtime::binaryen::roundUpAlign(size1 + ptr1));
}

/**
 * @given memory with allocated memory chunk
 * @when this memory is deallocated
 * @then the size of this memory chunk is returned
 */
TEST_F(MemoryHeapTest, DeallocateExisingMemoryChunk) {
  const size_t size1 = 3;

  auto ptr1 = allocate(size1);

  auto opt_deallocated_size = deallocate(ptr1);
  ASSERT_TRUE(opt_deallocated_size.has_value());
  ASSERT_EQ(*opt_deallocated_size, roundUpAlign(size1));
}

/**
 * @given memory with memory chunk allocated at the beginning
 * @when deallocate is invoked with ptr that does not point to any memory
 * chunk
 * @then deallocate returns none
 */
TEST_F(MemoryHeapTest, DeallocateNonexistingMemoryChunk) {
  const size_t size1 = 2047;

  allocate(size1);

  auto ptr_to_nonexisting_chunk = 2;
  auto opt_deallocated_size = deallocate(ptr_to_nonexisting_chunk);
  ASSERT_FALSE(opt_deallocated_size.has_value());
}

/**
 * @given memory with two memory chunk filling entire memory
 * @when first memory chunk of size size1 is deallocated @and new memory chunk
 * of the same size is trying to be allocated on that memory
 * @then it is allocated on the place of the first memory chunk
 */
TEST_F(MemoryHeapTest, AllocateAfterDeallocate) {
  // two memory sizes totalling to the total memory size
  const size_t size1 = 2035;
  const size_t size2 = 2037;

  // allocate two memory chunks with total size equal to the memory size
  auto pointer_of_first_allocation = allocate(size1);
  allocate(size2);

  // deallocate first memory chunk
  deallocate(pointer_of_first_allocation);

  // allocate new memory chunk
  auto pointer_of_repeated_allocation = allocate(size1);
  // expected that it will be allocated on the same place as the first memory
  // chunk that was deallocated
  ASSERT_EQ(pointer_of_first_allocation, pointer_of_repeated_allocation);
}

/**
 * @given full memory with deallocated memory chunk of size1
 * @when allocate memory chunk of size bigger than size1
 * @then allocate returns memory of size bigger
 */
TEST_F(MemoryHeapTest, AllocateTooBigMemoryAfterDeallocate) {
  // two memory sizes totalling to the total memory size
  const size_t size1 = 2047;
  const size_t size2 = 2049;

  // allocate two memory chunks with total size equal to the memory size
  auto ptr1 = allocate(size1);
  auto ptr2 = allocate(size2);

  // calculate memory offset after two allocations
  auto mem_offset = ptr2 + size2;

  // deallocate first memory chunk
  deallocate(ptr1);

  // allocate new memory chunk with bigger size than the space left in the
  // memory
  auto ptr3 = allocate(size1 + 1);

  // memory is allocated on mem offset (aligned by 4)
  ASSERT_EQ(ptr3, kagome::runtime::binaryen::roundUpAlign(mem_offset));
}

/**
 * @given full memory with different sized memory chunks
 * @when deallocate chunks with various way: in order, reversively, alone chunk
 * @then neighbor chunks is combined
 */
TEST_F(MemoryHeapTest, CombineDeallocatedChunks) {
  // Fill memory
  constexpr size_t size1 = roundUpAlign(1) * 1;
  auto ptr1 = allocate(size1);
  constexpr size_t size2 = roundUpAlign(1) * 2;
  auto ptr2 = allocate(size2);
  constexpr size_t size3 = roundUpAlign(1) * 3;
  auto ptr3 = allocate(size3);
  constexpr size_t size4 = roundUpAlign(1) * 4;
  auto ptr4 = allocate(size4);
  constexpr size_t size5 = roundUpAlign(1) * 5;
  auto ptr5 = allocate(size5);
  constexpr size_t size6 = roundUpAlign(1) * 6;
  auto ptr6 = allocate(size6);
  constexpr size_t size7 = roundUpAlign(1) * 7;
  auto ptr7 = allocate(size7);
  // A: [ 1 ][ 2 ][ 3 ][ 4 ][ 5 ][ 6 ][ 7 ]
  // D:

  deallocate(ptr2);
  // A: [ 1 ]     [ 3 ][ 4 ][ 5 ][ 6 ][ 7 ]
  // D:      [ 2 ]
  deallocate(ptr3);
  // A: [ 1 ]          [ 4 ][ 5 ][ 6 ][ 7 ]
  // D:      [ 2    3 ]
  {
    auto it = deallocated_.find(ptr2);
    ASSERT_TRUE(it != deallocated_.end());
    EXPECT_EQ(it->second, size2 + size3);
  }

  deallocate(ptr5);
  // A: [ 1 ]          [ 4 ]     [ 6 ][ 7 ]
  // D:      [ 2    3 ]     [ 5 ]
  deallocate(ptr6);
  // A: [ 1 ]          [ 4 ]          [ 7 ]
  // D:      [ 2    3 ]     [ 5    6 ]
  {
    auto it = deallocated_.find(ptr5);
    ASSERT_TRUE(it != deallocated_.end());
    EXPECT_EQ(it->second, size5 + size6);
  }

  deallocate(ptr4);
  // A: [ 1 ]                         [ 7 ]
  // D:      [ 2    3    4    5    6 ]
  {
    auto it = deallocated_.find(ptr2);
    ASSERT_TRUE(it != deallocated_.end());
    EXPECT_EQ(it->second, size2 + size3 + size4 + size5 + size6);
  }

  EXPECT_EQ(deallocated_.size(), 1);
  EXPECT_EQ(allocated_.size(), 2);
  EXPECT_NE(allocated_.find(ptr1), allocated_.end());
  EXPECT_NE(allocated_.find(ptr7), allocated_.end());
}

/**
 * @given arbitrary buffer of size N
 * @when this buffer is stored in memory heap @and then load of N bytes is done
 * @then the same buffer is returned
 */
TEST_F(MemoryHeapTest, LoadNTest) {
  const size_t N = 3;

  kagome::common::Buffer b(N, 'c');

  auto ptr = allocate(N);

  storeBuffer(ptr, b);

  auto res_b = loadN(ptr, N);
  ASSERT_EQ(b, res_b);
}

/**
 * @given Some memory is allocated
 * @when Memory is reset
 * @then Allocated memory's offset is min non-zero aligned address
 */
TEST_F(MemoryHeapTest, ResetTest) {
  const size_t N = 42;
  ASSERT_EQ(allocate(N), roundUpAlign(1));
  reset();
  ASSERT_EQ(allocate(N), roundUpAlign(1));
}
