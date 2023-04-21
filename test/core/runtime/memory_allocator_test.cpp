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
  static void SetUpTestCase() {
  }

  void SetUp() override {
    allocator_ = std::make_unique<runtime::MemoryAllocatorNew<8ull>>(512ull * 3ull);
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

    ASSERT_EQ(0xffffffffffffffff, allocator_->getEndingMask<true>(63, 5, remains));
    ASSERT_EQ(remains, 4);

    ASSERT_EQ(0x000000000000000f, allocator_->getEndingMask<true>(3, 1, remains));
    ASSERT_EQ(remains, 0);

    ASSERT_EQ(0xffffffffffffffff, allocator_->getEndingMask<true>(1, 64, remains));
    ASSERT_EQ(remains, 1);
}

TEST_F(MemoryAllocatorTest, EndingMaskSecondSegment) {
    size_t remains;

    ASSERT_EQ(0x000000000000001f, allocator_->getEndingMask<false>(1, 5, remains));
    ASSERT_EQ(0x000000000000001f, allocator_->getEndingMask<false>(1, 5, remains));
    ASSERT_EQ(0x0000000000000000, allocator_->getEndingMask<false>(10, 0, remains));
}

TEST_F(MemoryAllocatorTest, SegmentMaskFirstSegment) {
    size_t remains;

    ASSERT_EQ(0xffffffffffffffff, allocator_->getSegmentMask<true>(0, 64, remains));
    ASSERT_EQ(remains, 0);

    ASSERT_EQ(0x0000000000000ff0, allocator_->getSegmentMask<true>(4, 8, remains));
    ASSERT_EQ(remains, 0);

    ASSERT_EQ(0xfffffffffffffffe, allocator_->getSegmentMask<true>(1, 64, remains));
    ASSERT_EQ(remains, 1);

    ASSERT_EQ(0xf000000000000000, allocator_->getSegmentMask<true>(60, 4, remains));
    ASSERT_EQ(remains, 0);

    ASSERT_EQ(0xf000000000000000, allocator_->getSegmentMask<true>(60, 10, remains));
    ASSERT_EQ(remains, 6);
}

TEST_F(MemoryAllocatorTest, SegmentMaskSecondSegment) {
    size_t remains = 20;
    ASSERT_EQ(0x000000000000000f, allocator_->getSegmentMask<false>(10, 4, remains));
    ASSERT_EQ(0x0000000000000001, allocator_->getSegmentMask<false>(10, 1, remains));
    ASSERT_EQ(0x7fffffffffffffff, allocator_->getSegmentMask<false>(10, 63, remains));
    ASSERT_EQ(0xffffffffffffffff, allocator_->getSegmentMask<false>(10, 64, remains));
    ASSERT_EQ(0x0000000000000000, allocator_->getSegmentMask<false>(10, 0, remains));
}

TEST_F(MemoryAllocatorTest, SegmentFilter_0) {
    const uint64_t *segment = nullptr;
    uint64_t preprocessed = std::numeric_limits<uint64_t>::max();
    allocator_->updateSegmentFilter(segment, preprocessed, 0x0000000000000003, 0x0000000000000000);
    ASSERT_EQ(uintptr_t(segment), 0ull);
    ASSERT_EQ(0xfffffffffffffffc, preprocessed);
}

TEST_F(MemoryAllocatorTest, SegmentFilter_01) {
    const uint64_t *segment = nullptr;
    uint64_t preprocessed = std::numeric_limits<uint64_t>::max();
    allocator_->updateSegmentFilter(segment, preprocessed, 0x7fffffffffffffff, 0x0000000000000000);
    ASSERT_EQ(uintptr_t(segment), 0ull);
    ASSERT_EQ(0x8000000000000000, preprocessed);
}

TEST_F(MemoryAllocatorTest, SegmentFilter_1) {
    const uint64_t *segment = nullptr;
    uint64_t preprocessed = std::numeric_limits<uint64_t>::max();
    allocator_->updateSegmentFilter(segment, preprocessed, 0x0000000000000000, 0x0000000000000001);
    ASSERT_EQ(uintptr_t(segment), 8ull);
    ASSERT_EQ(0xfffffffffffffffe, preprocessed);
}

TEST_F(MemoryAllocatorTest, SegmentFilter_11) {
    const uint64_t *segment = nullptr;
    uint64_t preprocessed = std::numeric_limits<uint64_t>::max();
    allocator_->updateSegmentFilter(segment, preprocessed, 0x0000000000000003, 0x0000000000000003);
    ASSERT_EQ(uintptr_t(segment), 8ull);
    ASSERT_EQ(0xfffffffffffffffc, preprocessed);
}

TEST_F(MemoryAllocatorTest, SegmentFilter_12) {
    const uint64_t *segment = nullptr;
    uint64_t preprocessed = std::numeric_limits<uint64_t>::max();
    allocator_->updateSegmentFilter(segment, preprocessed, 0x0000000000000003, 0x0000000000000002);
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

TEST_F(MemoryAllocatorTest, SearchContBits_NoMem) {
    size_t remains;
    memset(&allocator_->table_[0], 0, sizeof(allocator_->table_[0]) * allocator_->table_.size());
    ASSERT_EQ(allocator_->end(), allocator_->searchContiguousBitPack(5, remains));
    ASSERT_EQ(5ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBits_Part0) {
    size_t remains;
    memset(&allocator_->table_[0], 0, sizeof(allocator_->table_[0]) * allocator_->table_.size());
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
