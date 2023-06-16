/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <cstdint>

#include "mock/core/host_api/host_api_mock.hpp"
#include "runtime/binaryen/memory_impl.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/common/memory_allocator.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;

using runtime::kDefaultHeapBase;
using runtime::kInitialMemorySize;
using runtime::MemoryAllocator;
using runtime::binaryen::MemoryImpl;

class BinaryenMemoryHeapTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    using std::string_literals::operator""s;

    auto host_api = std::make_shared<host_api::HostApiMock>();
    rei_ =
        std::make_unique<runtime::binaryen::RuntimeExternalInterface>(host_api);
    auto allocator = std::make_unique<MemoryAllocator>(
        MemoryAllocator::MemoryHandle{
            [this](auto size) { return memory_->resize(size); },
            [this] { return memory_->size(); },
            [this](auto addr, uint32_t value) {
              memory_->store32(addr, value);
            },
            [this](auto addr) { return memory_->load32u(addr); }},
        kDefaultHeapBase);
    allocator_ = allocator.get();
    memory_ =
        std::make_unique<MemoryImpl>(rei_->getMemory(), std::move(allocator));
  }

  void TearDown() override {
    memory_.reset();
    rei_.reset();
  }

  static const uint32_t memory_size_ = kInitialMemorySize;

  std::unique_ptr<runtime::binaryen::RuntimeExternalInterface> rei_;
  std::unique_ptr<MemoryImpl> memory_;
  MemoryAllocator *allocator_;
};

/**
 * @given memory of arbitrary size
 * @when trying to allocate memory of size 0
 * @then zero pointer is returned
 */
TEST_F(BinaryenMemoryHeapTest, Return0WhenSize0) {
  ASSERT_EQ(memory_->allocate(0), 0);
}

/**
 * @given memory of size memory_size_
 * @when trying to allocate memory of size bigger than memory_size_ but less
 * than max memory size
 * @then -1 is not returned by allocate method indicating that memory was
 * allocated
 */
TEST_F(BinaryenMemoryHeapTest, AllocatedMoreThanMemorySize) {
  const auto allocated_memory = memory_size_ + 1;
  ASSERT_NE(memory_->allocate(allocated_memory), -1);
}

/**
 * @given memory of size memory_size_ that is fully allocated
 * @when trying to allocate memory of size bigger than
 * (kMaxMemorySize-memory_size_)
 * @then -1 is not returned by allocate method indicating that memory was not
 * allocated
 */
TEST_F(BinaryenMemoryHeapTest, AllocatedTooBigMemoryFailed) {
  // fully allocate memory
  auto ptr1 = memory_->allocate(memory_size_);
  // check that ptr1 is not -1, thus memory was allocated
  ASSERT_NE(ptr1, -1);

  // The memory size that can be allocated is within interval (0, kMaxMemorySize
  // - memory_size_]. Trying to allocate more
  auto big_memory_size = MemoryImpl::kMaxMemorySize - memory_size_ + 1;
  ASSERT_EQ(memory_->allocate(big_memory_size), 0);
}

/**
 * @given memory with allocated memory chunk
 * @when this memory is deallocated
 * @then the size of this memory chunk is returned
 */
TEST_F(BinaryenMemoryHeapTest, DeallocateExisingMemoryChunk) {
  const size_t size1 = 3;

  auto ptr1 = memory_->allocate(size1);

  auto opt_deallocated_size = memory_->deallocate(ptr1);
  ASSERT_TRUE(opt_deallocated_size.has_value());
  ASSERT_EQ(*opt_deallocated_size, runtime::roundUpAlign(size1));
}

/**
 * @given memory with two memory chunk filling entire memory
 * @when first memory chunk of size size1 is deallocated @and new memory chunk
 * of the same size is trying to be allocated on that memory
 * @then it is allocated on the place of the first memory chunk
 */
TEST_F(BinaryenMemoryHeapTest, AllocateAfterDeallocate) {
  auto available_memory_size = kInitialMemorySize - kDefaultHeapBase;

  // two memory sizes totalling to the total memory size
  const size_t size1 = available_memory_size / 3 + 1;
  const size_t size2 = available_memory_size / 3 + 1;

  // allocate two memory chunks with total size equal to the memory size
  auto pointer_of_first_allocation = memory_->allocate(size1);
  memory_->allocate(size2);

  // deallocate first memory chunk
  memory_->deallocate(pointer_of_first_allocation);

  // allocate new memory chunk
  auto pointer_of_repeated_allocation = memory_->allocate(size1);
  // expected that it will be allocated on the same place as the first memory
  // chunk that was deallocated
  ASSERT_EQ(pointer_of_first_allocation, pointer_of_repeated_allocation);
}

/**
 * @given full memory with different sized memory chunks
 * @when deallocate chunks in various ways: in order, reversed, single chunk
 * @then neighbor chunks are combined
 */
TEST_F(BinaryenMemoryHeapTest, CombineDeallocatedChunks) {
  // Fill memory
  constexpr size_t size1 = runtime::roundUpAlign(1) * 1;
  auto ptr1 = memory_->allocate(size1);
  constexpr size_t size2 = runtime::roundUpAlign(1) * 2;
  auto ptr2 = memory_->allocate(size2);
  constexpr size_t size3 = runtime::roundUpAlign(1) * 3;
  auto ptr3 = memory_->allocate(size3);
  constexpr size_t size4 = runtime::roundUpAlign(1) * 4;
  auto ptr4 = memory_->allocate(size4);
  constexpr size_t size5 = runtime::roundUpAlign(1) * 5;
  auto ptr5 = memory_->allocate(size5);
  constexpr size_t size6 = runtime::roundUpAlign(1) * 6;
  auto ptr6 = memory_->allocate(size6);
  constexpr size_t size7 = runtime::roundUpAlign(1) * 7;
  auto ptr7 = memory_->allocate(size7);
  // A: [ 1 ][ 2 ][ 3 ][ 4 ][ 5 ][ 6 ][ 7 ]
  // D:

  memory_->deallocate(ptr2);
  // A: [ 1 ]     [ 3 ][ 4 ][ 5 ][ 6 ][ 7 ]
  // D:      [ 2 ]
  memory_->deallocate(ptr3);
  // A: [ 1 ]          [ 4 ][ 5 ][ 6 ][ 7 ]
  // D:      [ 2    3 ]

  memory_->deallocate(ptr5);
  // A: [ 1 ]          [ 4 ]     [ 6 ][ 7 ]
  // D:      [ 2    3 ]     [ 5 ]
  memory_->deallocate(ptr6);
  // A: [ 1 ]          [ 4 ]          [ 7 ]
  // D:      [ 2    3 ]     [ 5    6 ]

  memory_->deallocate(ptr4);
  // A: [ 1 ]                         [ 7 ]
  // D:      [ 2    3    4    5    6 ]

  EXPECT_EQ(allocator_->getDeallocatedChunksNum(), 5);
  EXPECT_EQ(allocator_->getAllocatedChunkSize(ptr1), size1);
  EXPECT_EQ(allocator_->getAllocatedChunkSize(ptr7), size7);
}

/**
 * @given arbitrary buffer of size N
 * @when this buffer is stored in memory heap @and then load of N bytes is done
 * @then the same buffer is returned
 */
TEST_F(BinaryenMemoryHeapTest, LoadNTest) {
  const size_t N = 3;

  kagome::common::Buffer b(N, 'c');

  auto ptr = memory_->allocate(N);

  memory_->storeBuffer(ptr, b);

  auto res_b = memory_->loadN(ptr, N);
  ASSERT_EQ(b, res_b);
}
