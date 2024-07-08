/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>

#include "common/buffer.hpp"
#include "mock/core/host_api/host_api_mock.hpp"
#include "runtime/binaryen/memory_impl.hpp"
#include "runtime/binaryen/runtime_external_interface.hpp"
#include "runtime/common/memory_allocator.hpp"
#include "runtime/memory.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;

using common::literals::operator""_MB;
using runtime::kDefaultHeapBase;
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

    runtime::MemoryConfig config{kDefaultHeapBase};
    auto handle = std::make_shared<MemoryImpl>(rei_->getMemory());
    auto allocator =
        std::make_unique<runtime::MemoryAllocatorImpl>(handle, config);
    allocator_ = allocator.get();
    memory_ = std::make_unique<runtime::Memory>(handle, std::move(allocator));
  }

  std::unique_ptr<runtime::binaryen::RuntimeExternalInterface> rei_;
  std::unique_ptr<runtime::Memory> memory_;
  runtime::MemoryAllocatorImpl *allocator_;
};

/**
 * @given memory of arbitrary size
 * @when trying to allocate memory of size 0
 * @then zero pointer is returned
 */
TEST_F(BinaryenMemoryHeapTest, Return0WhenSize0) {
  auto ptr = memory_->allocate(0);
  ASSERT_EQ(allocator_->getAllocatedChunkSize(ptr), 8);
}

/**
 * @given memory with allocated memory chunk
 * @when this memory is deallocated
 * @then the size of this memory chunk is returned
 */
TEST_F(BinaryenMemoryHeapTest, DeallocateExisingMemoryChunk) {
  const size_t size1 = 3;

  auto ptr1 = memory_->allocate(size1);

  ASSERT_EQ(allocator_->getAllocatedChunkSize(ptr1),
            runtime::roundUpAlign(size1));
  memory_->deallocate(ptr1);
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
  EXPECT_EQ(allocator_->getAllocatedChunkSize(ptr7),
            math::nextHighPowerOf2(size7));
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
