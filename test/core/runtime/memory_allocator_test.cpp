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
    allocator_ = std::make_unique<runtime::MemoryAllocatorNew<>>(512ull);
  }

  void TearDown() override {
    allocator_.reset();
  }

  std::unique_ptr<runtime::MemoryAllocatorNew<>> allocator_;
};

TEST_F(MemoryAllocatorTest, LeadingMask) {
    ASSERT_EQ(0x7f, allocator_->getLeadingMask(6, true));
    ASSERT_EQ(0xff, allocator_->getLeadingMask(7, true));
    ASSERT_EQ(0x01, allocator_->getLeadingMask(0, true));
    ASSERT_EQ(0xffffffffffffffff, allocator_->getLeadingMask(1, false));
}

TEST_F(MemoryAllocatorTest, EndingMask) {
    ASSERT_EQ(0xffffffffffffffc0, allocator_->getEndingMask(7, 2, true));
    ASSERT_EQ(0xffffffffffffffe0, allocator_->getEndingMask(6, 2, true));
    ASSERT_EQ(0xffffffffffffffff, allocator_->getEndingMask(6, 10, true));
    ASSERT_EQ(0xffffffffffffffff, allocator_->getEndingMask(6, 7, true));
    ASSERT_EQ(0xfffffffffffffffe, allocator_->getEndingMask(6, 6, true));
}

TEST_F(MemoryAllocatorTest, SegmentMask) {
    ASSERT_EQ(0xc0, (allocator_->getEndingMask(7, 2, true) & allocator_->getLeadingMask(7, true)));
    ASSERT_EQ(0x60, (allocator_->getEndingMask(6, 2, true) & allocator_->getLeadingMask(6, true)));
    ASSERT_EQ(0x03, (allocator_->getEndingMask(1, 5, true) & allocator_->getLeadingMask(1, true)));
}

TEST_F(MemoryAllocatorTest, SegmentMask2) {
    ASSERT_EQ(0xe000000000000000, (allocator_->getEndingMask(7, 3, false) & allocator_->getLeadingMask(7, false)));
    ASSERT_EQ(0xe000000000000000, (allocator_->getEndingMask(6, 3, false) & allocator_->getLeadingMask(7, false)));
}

TEST_F(MemoryAllocatorTest, SegmentFilter) {
    uint64_t filter = std::numeric_limits<uint64_t>::max();
    allocator_->updateSegmentFilter(filter, 63ull);
    ASSERT_EQ(0x3fffffffffffffff, filter);

    allocator_->updateSegmentFilter(filter, 7ull);
    ASSERT_EQ(0x3fffffffffffff3f, filter);

    allocator_->updateSegmentFilter(filter, 0ull);
    ASSERT_EQ(0x3fffffffffffff3e, filter);
}

TEST_F(MemoryAllocatorTest, SearchContBits0) {
    size_t remains;
    ASSERT_EQ(0ull, allocator_->searchContiguousBitPack(5, remains));
    ASSERT_EQ(0ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBits1) {
    size_t remains;
    allocator_->table_[0] &= ~(1ull << 63ull);
    ASSERT_EQ(1ull, allocator_->searchContiguousBitPack(5, remains));
    ASSERT_EQ(0ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBitsNoMem) {
    size_t remains;
    memset(&allocator_->table_[0], 0, sizeof(allocator_->table_[0]) * allocator_->table_.size());
    ASSERT_EQ(allocator_->end(), allocator_->searchContiguousBitPack(5, remains));
    ASSERT_EQ(5ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBitsPart) {
    size_t remains;
    memset(&allocator_->table_[0], 0, sizeof(allocator_->table_[0]) * allocator_->table_.size());
    allocator_->table_[7] |= 3;
    ASSERT_EQ(510, allocator_->searchContiguousBitPack(5, remains));
    ASSERT_EQ(3ull, remains);
}

TEST_F(MemoryAllocatorTest, SearchContBitsPart1) {
    size_t remains;
    allocator_->table_[0] = 0ull;
    ASSERT_EQ(64ull, allocator_->searchContiguousBitPack(5, remains));
    ASSERT_EQ(0ull, remains);
}
