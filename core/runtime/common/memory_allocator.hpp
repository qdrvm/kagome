/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP
#define KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP

#include <algorithm>
#include <map>
#include <optional>
#include <unordered_map>
#include <vector>

#include <unistd.h>
#include "common/literals.hpp"
#include "log/logger.hpp"
#include "primitives/math.hpp"
#include "runtime/memory.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {

  // Alignment for pointers, same with substrate:
  // https://github.com/paritytech/substrate/blob/743981a083f244a090b40ccfb5ce902199b55334/primitives/allocator/src/freeing_bump.rs#L56
  constexpr uint8_t kAlignment = sizeof(size_t);
  constexpr size_t kDefaultHeapBase = [] {
    using namespace kagome::common::literals;
    return 1_MB;  // 1Mb
  }();

  /**
   * Obtain closest multiple of kAlignment that is greater or equal to given
   * number
   * @tparam T T type of number
   * @param t given number
   * @return closest multiple
   */
  template <typename T>
  static constexpr T roundUpAlign(T t) {
    return math::roundUp<kAlignment>(t);
  }

  /**
   * Implementation of allocator for the runtime memory
   * Combination of monotonic and free-list allocator
   */
  class MemoryAllocator final {
   public:
    struct MemoryHandle {
      std::function<void(size_t)> resize;
      std::function<size_t()> getSize;
    };
    MemoryAllocator(MemoryHandle memory, WasmPointer heap_base);

    WasmPointer allocate(WasmSize size);
    std::optional<WasmSize> deallocate(WasmPointer ptr);

    template <typename T>
    bool checkAddress(WasmPointer addr) noexcept {
      return offset_ > addr and offset_ - addr >= sizeof(T);
    }

    bool checkAddress(WasmPointer addr, size_t size) noexcept {
      return offset_ > addr and offset_ - addr >= size;
    }

    /// following methods are needed mostly for testing purposes
    std::optional<WasmSize> getDeallocatedChunkSize(WasmPointer ptr) const;
    std::optional<WasmSize> getAllocatedChunkSize(WasmPointer ptr) const;
    size_t getAllocatedChunksNum() const;
    size_t getDeallocatedChunksNum() const;

   private:
    /**
     * Finds memory segment of given size among deallocated pieces of memory
     * and allocates a memory there
     * @param size of target memory
     * @return address of memory of given size, or -1 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer freealloc(WasmSize size);

    /**
     * Resize memory and allocate memory segment of given size
     * @param size memory size to be allocated
     * @return pointer to the allocated memory @or 0 if it is impossible to
     * allocate this amount of memory
     */
    WasmPointer growAlloc(WasmSize size);

    void resize(WasmSize size);

   private:
    MemoryHandle memory_;

    // map containing addresses of allocated MemoryImpl chunks
    std::unordered_map<WasmPointer, WasmSize> allocated_;

    // map containing addresses to the deallocated MemoryImpl chunks
    std::map<WasmPointer, WasmSize> deallocated_;

    // Offset on the tail of the last allocated MemoryImpl chunk
    size_t offset_;

    log::Logger logger_;
  };

#define BSR(val) __builtin_ctzll(val)
#define BSF(val) __builtin_clzll(val)

  template <size_t G, size_t A = 8ull>
  struct MemoryAllocatorNew {
    static constexpr size_t kSegmentInBits = 64ull;
    static constexpr size_t kAlignment = A;
    static constexpr size_t kGranularity = G;
    static constexpr size_t kSegmentSize = kGranularity * kSegmentInBits;
    static constexpr size_t kAllocationSize = kSegmentSize;

    static_assert((kAlignment & (kAlignment - 1)) == 0, "Power of 2!");
    static_assert((kGranularity % kAlignment) == 0,
                  "Granularity is multiple of Alignment");
    static_assert((kSegmentSize & (kSegmentSize - 1)) == 0, "Power of 2!");

    size_t allocate(size_t size) {
      if (0ull == size || size > kSegmentSize) {
        return 0ull;
      }
      const auto allocation_size =
          math::roundUp<kGranularity>(size + AllocationHeader::kHeaderSize);
      const auto bits_len = bitsPackLenFromSize(allocation_size);

      size_t remains;
      const auto position = searchContiguousBitPack(bits_len, remains);
      if (remains != 0ull) {
        storageAdjust(remains * kGranularity);
      }

      auto const first_modified_segment = setContiguousBits<0ull>(position, bits_len);
      if (first_modified_segment == cursor_ && table_[first_modified_segment] == 0ull) {
        ++cursor_;
      }
      auto *const header_ptr =
          (AllocationHeader *)(position * kGranularity + startAddr());
      header_ptr->count = bits_len;
      return position * kGranularity + AllocationHeader::kHeaderSize;
    }

    std::optional<size_t> deallocate(size_t offset) {
      const auto alloc_begin = offset - AllocationHeader::kHeaderSize;
      auto *const header_ptr =
          (AllocationHeader *)(alloc_begin + startAddr());
      
      assert((alloc_begin % kGranularity) == 0ull);
      const auto position = alloc_begin / kGranularity;

      auto const first_modified_segment = setContiguousBits<1ull>(position, header_ptr->count);
      cursor_ = std::min(cursor_, first_modified_segment);
      return header_ptr->count * kGranularity - AllocationHeader::kHeaderSize;
    }

    MemoryAllocatorNew(size_t preallocated) {
      storageAdjust(preallocated);
    }

    ~MemoryAllocatorNew() {
      free(storage_);
    }

#ifndef TEST_MODE
   private:
#endif  // TEST_MODE
    struct AllocationHeader {
      static constexpr size_t kHeaderSize =
          math::roundUp<kAlignment>(sizeof(AllocationHeader));
      size_t count;  // lenght of allocated data in kGranularity units
    };
    static_assert(AllocationHeader::kHeaderSize <= kGranularity,
                  "Size of header should be less or equal granularity. Because "
                  "65 bits always can be allocated in 2 8-byte words");
    static_assert((kGranularity % kAlignment) == 0ull,
                  "Should be multiple of Alignment");

    auto bitsPackLenFromSize(size_t size) {
      return size / kGranularity;
    }

    auto segmentsToSize(size_t val) {
      return val * kSegmentSize;
    }

    auto sizeToSegments(size_t size) {
      assert((size & (kSegmentSize - 1)) == 0ull);
      return size / kSegmentSize;
    }

    /// @brief  Search for contiguous bits of the present length
    /// @param count of bits pack
    /// @param remains number of bits outside the range
    /// @return index of the pack begins, -1 otherwise
    size_t searchContiguousBitPack(const size_t count, size_t &remains) const {
      assert(!table_.empty());
      const auto *const begin = table_.data();
      const auto *const end = table_.data() + table_.size();

      const auto *segment = &table_[cursor_];
      uint64_t preprocessed_segment_filter =
          std::numeric_limits<uint64_t>::max();

      size_t position = 0ull;
      remains = count;
      while (end != segment) {
        remains = count;
        const auto preprocessed_segment =
            (*segment & preprocessed_segment_filter);
        position = (preprocessed_segment == 0ull ? kSegmentInBits
                                                 : BSR(preprocessed_segment));

        if (__builtin_expect(position != kSegmentInBits, 1)) {
          const auto segment_mask_0 =
              getSegmentMask<true>(position, count, remains);
          const auto segment_mask_1 =
              getSegmentMask<false>(0ull, remains, remains);

          /// unexisted last segment always correct for all part
          const auto n_last_segment = (segment + 1ull) != end;
          const auto next_segment = n_last_segment
                                      ? *(segment + 1ull)
                                      : std::numeric_limits<uint64_t>::max();
          const auto segment_0_filter =
              (preprocessed_segment & segment_mask_0) ^ segment_mask_0;
          const auto segment_1_filter =
              (next_segment & segment_mask_1) ^ segment_mask_1;
          if (__builtin_expect((segment_0_filter | segment_1_filter) == 0ull,
                               0)) {
            remains &= (uint64_t(n_last_segment) - 1ull);
            break;
          }
          updateSegmentFilter(segment,
                              preprocessed_segment_filter,
                              segment_0_filter,
                              segment_1_filter);
        } else {
          ++segment;
          preprocessed_segment_filter = std::numeric_limits<uint64_t>::max();
        }
      }

      return (segment - begin) * kSegmentInBits + (position % kSegmentInBits);
    }

    size_t end() const {
      return table_.size() * kSegmentInBits;
    }

    size_t headerSize() const {
      return AllocationHeader::kHeaderSize;
    }

    const uint8_t *startAddr() const {
      return storage_;
    }

    template <size_t kValue>
    size_t setContiguousBits(size_t position, size_t count) {
      const auto segment_ix = position / kSegmentInBits;
      const auto bit_ix = position % kSegmentInBits;

      size_t remains;
      const auto segment_mask_0 = getSegmentMask<true>(bit_ix, count, remains);
      if constexpr (kValue == 0ull) {
        table_[segment_ix] &= ~segment_mask_0;
      } else {
        table_[segment_ix] |= segment_mask_0;
      }

      if (table_.size() > segment_ix + 1) {
        const auto segment_mask_1 =
            remains == 0ull ? 0ull
                            : getSegmentMask<false>(0ull, remains, remains);
        if constexpr (kValue == 0ull) {
          table_[segment_ix + 1] &= ~segment_mask_1;
        } else {
          table_[segment_ix + 1] |= segment_mask_1;
        }
      }
      return segment_ix;
    }

    void updateSegmentFilter(const uint64_t *&segment,
                             uint64_t &preprocessed_filter,
                             uint64_t segment_filter_0,
                             uint64_t segment_filter_1) const {
      /*
        If segment_filter_1 != 0ull => we move segment to the next and update
        filter up to segment_filter_1 state, otherwise update up to
        segment_filter_0
      */
      if (segment_filter_1 != 0ull) {
        preprocessed_filter = std::numeric_limits<uint64_t>::max()
                           << (kSegmentInBits - BSF(segment_filter_1));
        ++segment;
      } else if (segment_filter_0 != 0ull) {
        preprocessed_filter = std::numeric_limits<uint64_t>::max()
                           << (kSegmentInBits - BSF(segment_filter_0));
      } else {
        UNREACHABLE;
      }
    }

    template <bool first_segment>
    uint64_t getLeadingMask(const size_t position) const {
      if constexpr (first_segment) {
        return (std::numeric_limits<uint64_t>::max() << position);
      }
      return std::numeric_limits<uint64_t>::max();
    }

    template <bool first_segment>
    uint64_t getSegmentMask(const size_t position,
                            const size_t count,
                            size_t &remains) const {
      return (getLeadingMask<first_segment>(position)
              & getEndingMask<first_segment>(position, count, remains));
    }

    template <bool first_segment>
    uint64_t getEndingMask(const size_t position,
                           const size_t count,
                           size_t &remains) const {
      if constexpr (first_segment) {
        if (position + count >= kSegmentInBits) {
          remains = count - (kSegmentInBits - position);
          return std::numeric_limits<uint64_t>::max();
        } else {
          remains = 0ull;
          return (std::numeric_limits<uint64_t>::max()
                  >> (kSegmentInBits - position - count));
        }
      } else {
        return count == 0ull ? 0ull
                             : (std::numeric_limits<uint64_t>::max()
                                >> (kSegmentInBits - count));
      }
    }

    void storageAdjust(size_t size) {
      const auto was_allocated = segmentsToSize(table_.size());
      const auto added_size = math::roundUpRuntime(size, kAllocationSize);
      const auto new_size = added_size + was_allocated;

      if (__builtin_expect(new_size <= Memory::kMaxMemorySize, 1)) {
        storage_ = (uint8_t *)std::realloc(storage_, new_size);
        table_.resize(table_.size() + sizeToSegments(added_size),
                      std::numeric_limits<uint64_t>::max());
      }
    }

    std::vector<uint64_t> table_{};
    uint8_t *storage_ = nullptr;
    size_t cursor_ = 0ull;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP
