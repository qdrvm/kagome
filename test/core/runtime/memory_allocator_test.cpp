/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <memory>

#define TEST_MODE
#include "runtime/common/memory_allocator.hpp"

using namespace kagome;

class MemoryAllocatorTest : public ::testing::Test {
 protected:
  static void SetUpTestCase() {}

  void SetUp() override {
    allocator_ =
        std::make_unique<runtime::MemoryAllocatorNew<8ull>>(512ull * 3ull);
  }

  void TearDown() override {
    allocator_.reset();
  }

  std::unique_ptr<runtime::MemoryAllocatorNew<8ull>> allocator_;
};

TEST_F(MemoryAllocatorTest, LeadingMaskFirstSegment) {
  ASSERT_EQ(0xffffffffffffffff, allocator_->getLeadingMask<true>(0));
  ASSERT_EQ(0xfffffffffffffffe, allocator_->getLeadingMask<true>(1));
  ASSERT_EQ(0xfffffffffffffff0, allocator_->getLeadingMask<true>(4));
  ASSERT_EQ(0x8000000000000000, allocator_->getLeadingMask<true>(63));
}

TEST_F(MemoryAllocatorTest, LeadingMaskSecondSegment) {
  ASSERT_EQ(0xffffffffffffffff, allocator_->getLeadingMask<false>(0));
  ASSERT_EQ(0xffffffffffffffff, allocator_->getLeadingMask<false>(5));
  ASSERT_EQ(0xffffffffffffffff, allocator_->getLeadingMask<false>(63));
}

TEST_F(MemoryAllocatorTest, EndingMaskFirstSegment) {
  size_t remains;

  ASSERT_EQ(0x000000000000001f, allocator_->getEndingMask<true>(0, 5, remains));
  ASSERT_EQ(remains, 0);

  ASSERT_EQ(0x00000000000000ff, allocator_->getEndingMask<true>(4, 4, remains));
  ASSERT_EQ(remains, 0);

  ASSERT_EQ(0xffffffffffffffff,
            allocator_->getEndingMask<true>(63, 5, remains));
  ASSERT_EQ(remains, 4);

  ASSERT_EQ(0x000000000000000f, allocator_->getEndingMask<true>(3, 1, remains));
  ASSERT_EQ(remains, 0);

  ASSERT_EQ(0xffffffffffffffff,
            allocator_->getEndingMask<true>(1, 64, remains));
  ASSERT_EQ(remains, 1);
}

TEST_F(MemoryAllocatorTest, EndingMaskSecondSegment) {
  size_t remains;

  ASSERT_EQ(0x000000000000001f,
            allocator_->getEndingMask<false>(1, 5, remains));
  ASSERT_EQ(0x000000000000001f,
            allocator_->getEndingMask<false>(1, 5, remains));
  ASSERT_EQ(0x0000000000000000,
            allocator_->getEndingMask<false>(10, 0, remains));
}

TEST_F(MemoryAllocatorTest, SegmentMaskComplex_0) {
  size_t remains;
  ASSERT_EQ(0xffffffffffffffff,
            allocator_->getSegmentMask<true>(0, 64, remains));
  ASSERT_EQ(remains, 0);

  ASSERT_EQ(0x0000000000000000,
            allocator_->getSegmentMask<false>(4, remains, remains));
  ASSERT_EQ(remains, 0);
}

TEST_F(MemoryAllocatorTest, SegmentMaskComplex_1) {
  size_t remains;
  ASSERT_EQ(0xfffffffffffffffe,
            allocator_->getSegmentMask<true>(1, 64, remains));
  ASSERT_EQ(remains, 1);

  ASSERT_EQ(0x0000000000000001,
            allocator_->getSegmentMask<false>(4, remains, remains));
  ASSERT_EQ(remains, 1);
}

TEST_F(MemoryAllocatorTest, SegmentMaskFirstSegment) {
  size_t remains;

  ASSERT_EQ(0xffffffffffffffff,
            allocator_->getSegmentMask<true>(0, 64, remains));
  ASSERT_EQ(remains, 0);

  ASSERT_EQ(0x0000000000000ff0,
            allocator_->getSegmentMask<true>(4, 8, remains));
  ASSERT_EQ(remains, 0);

  ASSERT_EQ(0xfffffffffffffffe,
            allocator_->getSegmentMask<true>(1, 64, remains));
  ASSERT_EQ(remains, 1);

  ASSERT_EQ(0xf000000000000000,
            allocator_->getSegmentMask<true>(60, 4, remains));
  ASSERT_EQ(remains, 0);

  ASSERT_EQ(0xf000000000000000,
            allocator_->getSegmentMask<true>(60, 10, remains));
  ASSERT_EQ(remains, 6);
}

TEST_F(MemoryAllocatorTest, SegmentMaskSecondSegment) {
  size_t remains = 20;
  ASSERT_EQ(0x000000000000000f,
            allocator_->getSegmentMask<false>(10, 4, remains));
  ASSERT_EQ(0x0000000000000001,
            allocator_->getSegmentMask<false>(10, 1, remains));
  ASSERT_EQ(0x7fffffffffffffff,
            allocator_->getSegmentMask<false>(10, 63, remains));
  ASSERT_EQ(0xffffffffffffffff,
            allocator_->getSegmentMask<false>(10, 64, remains));
  ASSERT_EQ(0x0000000000000000,
            allocator_->getSegmentMask<false>(10, 0, remains));
}

TEST_F(MemoryAllocatorTest, SegmentFilter_0) {
  const uint64_t *segment = nullptr;
  uint64_t preprocessed = std::numeric_limits<uint64_t>::max();
  allocator_->updateSegmentFilter(
      segment, preprocessed, 0x0000000000000003, 0x0000000000000000);
  ASSERT_EQ(uintptr_t(segment), 0ull);
  ASSERT_EQ(0xfffffffffffffffc, preprocessed);
}

TEST_F(MemoryAllocatorTest, SegmentFilter_01) {
  const uint64_t *segment = nullptr;
  uint64_t preprocessed = std::numeric_limits<uint64_t>::max();
  allocator_->updateSegmentFilter(
      segment, preprocessed, 0x7fffffffffffffff, 0x0000000000000000);
  ASSERT_EQ(uintptr_t(segment), 0ull);
  ASSERT_EQ(0x8000000000000000, preprocessed);
}

TEST_F(MemoryAllocatorTest, SegmentFilter_1) {
  const uint64_t *segment = nullptr;
  uint64_t preprocessed = std::numeric_limits<uint64_t>::max();
  allocator_->updateSegmentFilter(
      segment, preprocessed, 0x0000000000000000, 0x0000000000000001);
  ASSERT_EQ(uintptr_t(segment), 8ull);
  ASSERT_EQ(0xfffffffffffffffe, preprocessed);
}

TEST_F(MemoryAllocatorTest, SegmentFilter_11) {
  const uint64_t *segment = nullptr;
  uint64_t preprocessed = std::numeric_limits<uint64_t>::max();
  allocator_->updateSegmentFilter(
      segment, preprocessed, 0x0000000000000003, 0x0000000000000003);
  ASSERT_EQ(uintptr_t(segment), 8ull);
  ASSERT_EQ(0xfffffffffffffffc, preprocessed);
}

TEST_F(MemoryAllocatorTest, SegmentFilter_12) {
  const uint64_t *segment = nullptr;
  uint64_t preprocessed = std::numeric_limits<uint64_t>::max();
  allocator_->updateSegmentFilter(
      segment, preprocessed, 0x0000000000000003, 0x0000000000000002);
  ASSERT_EQ(uintptr_t(segment), 8ull);
  ASSERT_EQ(0xfffffffffffffffc, preprocessed);
}

TEST_F(MemoryAllocatorTest, SearchContBits_0) {
  size_t remains;
  ASSERT_EQ(0ull, allocator_->searchContiguousBitPack(5, remains));
  ASSERT_EQ(0ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBits_1) {
  size_t remains;
  allocator_->table_[0] &= ~1ull;
  ASSERT_EQ(1ull, allocator_->searchContiguousBitPack(5, remains));
  ASSERT_EQ(0ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBits_NoMem_0) {
  size_t remains;
  memset(&allocator_->table_[0],
         0,
         sizeof(allocator_->table_[0]) * allocator_->table_.size());
  ASSERT_EQ(allocator_->end(), allocator_->searchContiguousBitPack(5, remains));
  ASSERT_EQ(5ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBits_NoMem_1) {
  size_t remains;
  memset(&allocator_->table_[0],
         0,
         sizeof(allocator_->table_[0]) * allocator_->table_.size());
  ASSERT_EQ(allocator_->end(),
            allocator_->searchContiguousBitPack(64, remains));
  ASSERT_EQ(64, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBits_Part0) {
  size_t remains;
  memset(&allocator_->table_[0],
         0,
         sizeof(allocator_->table_[0]) * allocator_->table_.size());
  allocator_->table_[2] |= 0xc000000000000000;
  ASSERT_EQ(190, allocator_->searchContiguousBitPack(5, remains));
  ASSERT_EQ(3ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBits_Part1) {
  size_t remains;
  allocator_->table_[0] = 0ull;
  ASSERT_EQ(64ull, allocator_->searchContiguousBitPack(5, remains));
  ASSERT_EQ(0ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBits_Part3) {
  size_t remains;
  allocator_->table_[0] = 0x8000000000000000;
  ASSERT_EQ(63ull, allocator_->searchContiguousBitPack(64, remains));
  ASSERT_EQ(0ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBits_Part4) {
  size_t remains;
  allocator_->table_[0] = 0x0000000000000000;
  ASSERT_EQ(64ull, allocator_->searchContiguousBitPack(64, remains));
  ASSERT_EQ(0ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBits_Part5) {
  size_t remains;
  allocator_->table_[0] = 0x0000000000000000;
  allocator_->table_[1] = 0x0000000000000000;
  ASSERT_EQ(128ull, allocator_->searchContiguousBitPack(64, remains));
  ASSERT_EQ(0ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBits_Part6) {
  size_t remains;
  allocator_->table_[0] = 0x0000000000000000;
  allocator_->table_[1] = 0x0000000000000000;
  allocator_->table_[2] = 0xfffffffffffffffe;
  ASSERT_EQ(129ull, allocator_->searchContiguousBitPack(64, remains));
  ASSERT_EQ(1ull, remains);
}

TEST_F(MemoryAllocatorTest, AllocateTest_0) {
  const auto ptr = allocator_->allocate(1ull + allocator_->kGranularity
                                        - allocator_->headerSize());
  ASSERT_EQ(allocator_->headerSize(), ptr);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->deallocate(ptr);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, AllocateTest_1) {
  allocator_->table_[0] = 0x0000000000000000;
  allocator_->table_[1] = 0x0000000000000000;
  const auto ptr = allocator_->allocate(2 * allocator_->kGranularity
                                        - allocator_->headerSize());
  ASSERT_EQ(allocator_->kSegmentInBits * allocator_->kGranularity * 2ull
                + allocator_->headerSize(),
            ptr);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[2]);

  allocator_->deallocate(ptr);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, AllocateTest_2) {
  const auto ptr =
      allocator_->allocate(allocator_->kSegmentSize - allocator_->headerSize());
  ASSERT_EQ(allocator_->headerSize(), ptr);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->deallocate(ptr);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, AllocateTest_3) {
  const auto ptr_0 = allocator_->allocate(60ull * allocator_->kGranularity
                                          - allocator_->headerSize());
  ASSERT_EQ(allocator_->headerSize(), ptr_0);
  ASSERT_EQ(0xf000000000000000, allocator_->table_[0]);

  const auto ptr_1 = allocator_->allocate(12ull * allocator_->kGranularity
                                          - allocator_->headerSize());
  ASSERT_EQ(60ull * allocator_->kGranularity + allocator_->headerSize(), ptr_1);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffff00, allocator_->table_[1]);

  const auto ptr_2 = allocator_->allocate(56ull * allocator_->kGranularity
                                          - allocator_->headerSize());
  ASSERT_EQ(
      (60ull + 12ull) * allocator_->kGranularity + allocator_->headerSize(),
      ptr_2);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_3 = allocator_->allocate(64ull * allocator_->kGranularity
                                          - allocator_->headerSize());
  ASSERT_EQ(128ull * allocator_->kGranularity + allocator_->headerSize(),
            ptr_3);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(3ull, allocator_->table_.size());

  const auto ptr_4 = allocator_->allocate(1ull + allocator_->kGranularity
                                          - allocator_->headerSize());
  ASSERT_EQ(192ull * allocator_->kGranularity + allocator_->headerSize(),
            ptr_4);
  ASSERT_EQ(4ull, allocator_->table_.size());
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[3]);

  const auto ptr_5 = allocator_->allocate(58ull * allocator_->kGranularity
                                          - allocator_->headerSize());
  ASSERT_EQ(194ull * allocator_->kGranularity + allocator_->headerSize(),
            ptr_5);
  ASSERT_EQ(4ull, allocator_->table_.size());
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xf000000000000000, allocator_->table_[3]);

  const auto ptr_6 = allocator_->allocate(8ull * allocator_->kGranularity
                                          - allocator_->headerSize());
  ASSERT_EQ(252ull * allocator_->kGranularity + allocator_->headerSize(),
            ptr_6);
  ASSERT_EQ(5ull, allocator_->table_.size());
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[3]);
  ASSERT_EQ(0xfffffffffffffff0, allocator_->table_[4]);

  const auto ptr_7 = allocator_->allocate(60ull * allocator_->kGranularity
                                          - allocator_->headerSize());
  ASSERT_EQ(260ull * allocator_->kGranularity + allocator_->headerSize(),
            ptr_7);
  ASSERT_EQ(5ull, allocator_->table_.size());
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[3]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[4]);

  const auto ptr_8 = allocator_->allocate(64ull * allocator_->kGranularity
                                          - allocator_->headerSize());
  ASSERT_EQ(320ull * allocator_->kGranularity + allocator_->headerSize(),
            ptr_8);
  ASSERT_EQ(6ull, allocator_->table_.size());
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[3]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[4]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[5]);

  allocator_->deallocate(ptr_6);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xf000000000000000, allocator_->table_[3]);
  ASSERT_EQ(0x000000000000000f, allocator_->table_[4]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[5]);

  allocator_->deallocate(ptr_1);
  ASSERT_EQ(0xf000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x00000000000000ff, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xf000000000000000, allocator_->table_[3]);
  ASSERT_EQ(0x000000000000000f, allocator_->table_[4]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[5]);

  allocator_->deallocate(ptr_0);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0x00000000000000ff, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xf000000000000000, allocator_->table_[3]);
  ASSERT_EQ(0x000000000000000f, allocator_->table_[4]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[5]);

  allocator_->deallocate(ptr_4);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0x00000000000000ff, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xf000000000000003, allocator_->table_[3]);
  ASSERT_EQ(0x000000000000000f, allocator_->table_[4]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[5]);

  allocator_->deallocate(ptr_5);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0x00000000000000ff, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[3]);
  ASSERT_EQ(0x000000000000000f, allocator_->table_[4]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[5]);

  allocator_->deallocate(ptr_2);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[3]);
  ASSERT_EQ(0x000000000000000f, allocator_->table_[4]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[5]);

  allocator_->deallocate(ptr_7);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[3]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[4]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[5]);

  allocator_->deallocate(ptr_8);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[3]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[4]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[5]);

  allocator_->deallocate(ptr_3);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[3]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[4]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[5]);
}

TEST_F(MemoryAllocatorTest, Allocate65Test_0) {
  const auto ptr = allocator_->allocate(allocator_->kSegmentSize);
  ASSERT_EQ(allocator_->headerSize(), ptr);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xfffffffffffffffe, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->deallocate(ptr);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, Allocate65Test_1) {
  const auto ptr_0 = allocator_->allocate(allocator_->kSegmentSize);
  ASSERT_EQ(allocator_->headerSize(), ptr_0);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xfffffffffffffffe, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_1 = allocator_->allocate(allocator_->kSegmentSize);
  ASSERT_EQ(65ull * allocator_->kGranularity + allocator_->headerSize(), ptr_1);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[2]);

  allocator_->deallocate(ptr_0);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000001, allocator_->table_[1]);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, Allocate65Test_2) {
  const auto ptr_0 =
      allocator_->allocate(allocator_->kSegmentSize - allocator_->headerSize());
  ASSERT_EQ(allocator_->headerSize(), ptr_0);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_1 =
      allocator_->allocate(allocator_->kSegmentSize - allocator_->headerSize());
  ASSERT_EQ(64ull * allocator_->kGranularity + allocator_->headerSize(), ptr_1);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_2 =
      allocator_->allocate(allocator_->kSegmentSize - allocator_->headerSize());
  ASSERT_EQ(128ull * allocator_->kGranularity + allocator_->headerSize(),
            ptr_2);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);

  allocator_->deallocate(ptr_1);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);

  const auto ptr_3 = allocator_->allocate(allocator_->kSegmentSize);
  ASSERT_EQ(192ull * allocator_->kGranularity + allocator_->headerSize(),
            ptr_3);
  ASSERT_EQ(5ull, allocator_->table_.size());
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[3]);
  ASSERT_EQ(0xfffffffffffffffe, allocator_->table_[4]);

  const auto ptr_4 =
      allocator_->allocate(allocator_->kSegmentSize - allocator_->headerSize());
  ASSERT_EQ(64ull * allocator_->kGranularity + allocator_->headerSize(), ptr_4);
  ASSERT_EQ(5ull, allocator_->table_.size());
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[3]);
  ASSERT_EQ(0xfffffffffffffffe, allocator_->table_[4]);

  const auto ptr_5 = allocator_->allocate(1ull);
  ASSERT_EQ(
      (4 * 64ull + 1) * allocator_->kGranularity + allocator_->headerSize(),
      ptr_5);
  ASSERT_EQ(5ull, allocator_->table_.size());
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[3]);
  ASSERT_EQ(0xfffffffffffffff8, allocator_->table_[4]);

  allocator_->deallocate(ptr_0);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[3]);
  ASSERT_EQ(0xfffffffffffffff8, allocator_->table_[4]);

  allocator_->deallocate(ptr_3);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[3]);
  ASSERT_EQ(0xfffffffffffffff9, allocator_->table_[4]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_1) {
  const auto ptr_0 = allocator_->realloc(0ull, allocator_->kGranularity);
  ASSERT_EQ(0ull + allocator_->headerSize(), ptr_0);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->deallocate(ptr_0);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_2) {
  const auto ptr_0 = allocator_->realloc(0ull, allocator_->kGranularity);
  ASSERT_EQ(0ull + allocator_->headerSize(), ptr_0);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_1 = allocator_->realloc(ptr_0, allocator_->kGranularity);
  ASSERT_EQ(ptr_1, ptr_0);
  ASSERT_EQ(allocator_->getHeader(ptr_1).count, 2);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->deallocate(ptr_0);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_3) {
  const auto ptr_0 = allocator_->realloc(0ull, allocator_->kGranularity);
  ASSERT_EQ(0ull + allocator_->headerSize(), ptr_0);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_1 = allocator_->realloc(ptr_0, 2 * allocator_->kGranularity);
  ASSERT_EQ(ptr_1, ptr_0);
  ASSERT_EQ(allocator_->getHeader(ptr_1).count, 3);
  ASSERT_EQ(0xfffffffffffffff8, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->deallocate(ptr_0);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_4) {
  const auto ptr_0 = allocator_->allocate(allocator_->kGranularity);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_1 = allocator_->allocate(allocator_->kGranularity);
  ASSERT_EQ(0xfffffffffffffff0, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  [[maybe_unused]] const auto ptr_2 =
      allocator_->allocate(allocator_->kGranularity);
  ASSERT_EQ(0xffffffffffffffc0, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->deallocate(ptr_1);
  ASSERT_EQ(0xffffffffffffffcc, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_3 = allocator_->realloc(ptr_0, 3 * allocator_->kGranularity);
  ASSERT_EQ(ptr_3, ptr_0);
  ASSERT_EQ(0xffffffffffffffc0, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_5) {
  const auto ptr_0 = allocator_->realloc(0ull, allocator_->kSegmentSize + 1);
  ASSERT_EQ(0ull, ptr_0);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_6) {
  const auto ptr_0 = allocator_->allocate(allocator_->kGranularity);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_1 = allocator_->allocate(allocator_->kGranularity);
  ASSERT_EQ(0xfffffffffffffff0, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  [[maybe_unused]] const auto ptr_2 =
      allocator_->allocate(allocator_->kGranularity);
  ASSERT_EQ(0xffffffffffffffc0, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->deallocate(ptr_1);
  ASSERT_EQ(0xffffffffffffffcc, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->realloc(ptr_0, 4 * allocator_->kGranularity);
  ASSERT_EQ(0xfffffffffffff80f, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_7) {
  ASSERT_EQ(sizeof(uint64_t), allocator_->kGranularity);

  const auto ptr_0 = allocator_->allocate(allocator_->kGranularity);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  *(uint64_t *)allocator_->toAddr(ptr_0) = 0x1234567812345678ull;

  const auto ptr_1 = allocator_->allocate(allocator_->kGranularity);
  ASSERT_EQ(0xfffffffffffffff0, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  [[maybe_unused]] const auto ptr_2 =
      allocator_->allocate(allocator_->kGranularity);
  ASSERT_EQ(0xffffffffffffffc0, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->deallocate(ptr_1);
  ASSERT_EQ(0xffffffffffffffcc, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_3 = allocator_->realloc(ptr_0, 4 * allocator_->kGranularity);
  ASSERT_EQ(0xfffffffffffff80f, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
  ASSERT_EQ(*(uint64_t *)allocator_->toAddr(ptr_3), 0x1234567812345678ull);
}

TEST_F(MemoryAllocatorTest, RelocateTest_8) {
  allocator_->allocate(allocator_->kGranularity * 31);
  ASSERT_EQ(0xffffffff00000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_1 = allocator_->allocate(allocator_->kGranularity * 31);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_3 = allocator_->realloc(ptr_1, allocator_->kGranularity * 64);
  ASSERT_EQ(ptr_1, ptr_3);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xfffffffe00000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->deallocate(ptr_3);
  ASSERT_EQ(0xffffffff00000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_9) {
  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_1 = allocator_->allocate(allocator_->kGranularity * 31);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffff00000000, allocator_->table_[2]);

  const auto ptr_3 = allocator_->realloc(ptr_1, allocator_->kGranularity * 63);
  ASSERT_EQ(ptr_1, ptr_3);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_10) {
  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_1 = allocator_->allocate(allocator_->kGranularity * 31);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffff00000000, allocator_->table_[2]);

  const auto ptr_3 = allocator_->realloc(ptr_1, allocator_->kGranularity * 64);
  ASSERT_EQ(ptr_1, ptr_3);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xfffffffffffffffe, allocator_->table_[3]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_11) {
  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto ptr_1 = allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);

  const auto ptr_3 = allocator_->realloc(ptr_1, allocator_->kGranularity * 64);
  ASSERT_EQ(ptr_1, ptr_3);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xfffffffffffffffe, allocator_->table_[3]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_12) {
  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);

  allocator_->realloc(0ull, allocator_->kGranularity * 64);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[3]);
  ASSERT_EQ(0xfffffffffffffffe, allocator_->table_[4]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_13) {
  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->allocate(allocator_->kGranularity * 62);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x8000000000000000, allocator_->table_[2]);

  allocator_->realloc(0ull, allocator_->kGranularity * 64);
  ASSERT_EQ(allocator_->table_.size(), 4);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[3]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_14) {
  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->allocate(allocator_->kGranularity * 61);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xc000000000000000, allocator_->table_[2]);

  const auto p_1 = allocator_->realloc(0ull, allocator_->kGranularity);
  ASSERT_EQ(allocator_->table_.size(), 3);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);

  const auto p_2 = allocator_->realloc(p_1, allocator_->kGranularity * 64);
  ASSERT_EQ(allocator_->table_.size(), 4);
  ASSERT_EQ(p_1, p_2);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0x8000000000000000, allocator_->table_[3]);
}

TEST_F(MemoryAllocatorTest, AllocateWithGap_1) {
  const auto ptr_0 = allocator_->allocate(168);
  ASSERT_EQ(0xffffffffffc00000, allocator_->table_[0]);

  const auto ptr_1 = allocator_->allocate(168);
  ASSERT_EQ(0xfffff00000000000, allocator_->table_[0]);

  const auto ptr_2 = allocator_->allocate(168);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[1]);

  allocator_->deallocate(ptr_1);
  ASSERT_EQ(0x00000fffffc00000, allocator_->table_[0]);
  ASSERT_EQ(0xfffffffffffffffc, allocator_->table_[1]);

  const auto ptr_3 = allocator_->allocate(169);
  ASSERT_EQ(0x00000fffffc00000, allocator_->table_[0]);
  ASSERT_EQ(0xfffffffffe000000, allocator_->table_[1]);
}

TEST_F(MemoryAllocatorTest, RelocateTest_15) {
  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  const auto p_0 = allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[2]);

  allocator_->allocate(allocator_->kGranularity * 63);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);

  allocator_->deallocate(p_0);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);

  const auto p_1 = allocator_->realloc(0ull, 31 * allocator_->kGranularity);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffffff00000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);

  const auto p_2 = allocator_->realloc(p_1, 39 * allocator_->kGranularity);
  ASSERT_EQ(p_2, p_1);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xffffff0000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  strcpy((char *)allocator_->toAddr(p_2), "This is a test str!!!(c)");

  allocator_->realloc(0ull, 3 * allocator_->kGranularity);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xfffff00000000000, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);

  const auto p_3 = allocator_->realloc(p_2, 40 * allocator_->kGranularity);
  ASSERT_NE(p_3, p_2);
  ASSERT_EQ(allocator_->table_.size(), 4);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[0]);
  ASSERT_EQ(0xfffff0ffffffffff, allocator_->table_[1]);
  ASSERT_EQ(0x0000000000000000, allocator_->table_[2]);
  ASSERT_EQ(0xfffffe0000000000, allocator_->table_[3]);
  ASSERT_EQ(strcmp((char *)allocator_->toAddr(p_3), "This is a test str!!!(c)"),
            0);
}

TEST_F(MemoryAllocatorTest, AllocateTest_NoPreAlloc) {
  allocator_ = std::make_unique<runtime::MemoryAllocatorNew<8ull>>(0ull);
  ASSERT_EQ(allocator_->table_.size(), 0);
}

TEST_F(MemoryAllocatorTest, AllocateTest_Capacity_0) {
  allocator_ = std::make_unique<runtime::MemoryAllocatorNew<8ull>>(0ull);
  ASSERT_EQ(allocator_->capacity(), 0);
}

TEST_F(MemoryAllocatorTest, AllocateTest_Capacity_1) {
  allocator_ = std::make_unique<runtime::MemoryAllocatorNew<8ull>>(0ull);

  allocator_->allocate(1ull);
  ASSERT_EQ(allocator_->capacity(), 512);
}

TEST_F(MemoryAllocatorTest, AllocateTest_Capacity_512) {
  allocator_ = std::make_unique<runtime::MemoryAllocatorNew<8ull>>(0ull);

  allocator_->allocate(512ull);
  ASSERT_EQ(allocator_->capacity(), 1024);
}

TEST_F(MemoryAllocatorTest, AllocateTest_Size_1) {
  const auto ptr = allocator_->allocate(1ull);
  ASSERT_EQ(allocator_->size(ptr), 8ull);
}

TEST_F(MemoryAllocatorTest, AllocateTest_Size_512) {
  const auto ptr = allocator_->allocate(512ull);
  ASSERT_EQ(allocator_->size(ptr), 512ull);
}

TEST_F(MemoryAllocatorTest, GenericAllocator_Allocate) {
  allocator_.reset();
  runtime::GenericAllocator a_{100};
  // ASSERT_EQ(a_.);
}

struct A1 {
  int p;
};
struct A2 {
  int p;
};
struct A3 {
  int p;
};

/*template<typename F>
bool for_each_layer(std::tuple<A1, A2, A3> &layers_, F &&func) {
  bool found = false;
  auto f_result = [&] (auto &value) {
    found |= func(value);
  };

  std::apply([&](auto &...value)
  {
    (..., (f_result(value)));
  }, layers_);
  return found;
}*/

template <typename F>
bool for_each_layer(std::tuple<A1, A2, A3> &layers_, F &&func) {
  return std::apply(
      [&](auto &...value) {
        bool found = false;
        (..., (found |= func(value)));
        return found;
      },
      layers_);
}

TEST_F(MemoryAllocatorTest, AllocateTest_Layers) {
  auto layers{std::make_tuple(A1{1}, A2{2}, A3{3})};

  auto r = for_each_layer(layers, [](auto &l) {
    if (l.p < 5) {
      return false;
    }
    std::cerr << l.p << std::endl;
    return true;
  });
  std::cerr << "r: " << r << std::endl;
}
