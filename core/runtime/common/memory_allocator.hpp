/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP
#define KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP

#include <map>
#include <unordered_map>
#include <vector>
#include <optional>
#include <algorithm>

#include "common/literals.hpp"
#include "log/logger.hpp"
#include "primitives/math.hpp"
#include "runtime/types.hpp"
#include <unistd.h>
#include "runtime/memory.hpp"

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

  #define BSF(val) __builtin_clzll(val)

  template<size_t Align = 8ull>
  struct MemoryAllocatorNew {
    static constexpr size_t kSegmentInBits = 64ull;
    static constexpr size_t kAlignment = Align;
    static constexpr size_t kSegmentSize = kAlignment * kSegmentInBits;
    static constexpr size_t kPageSize = kSegmentSize * 4ull;

    static_assert((kAlignment & (kAlignment - 1)) == 0, "Power of 2!");
    static_assert((kSegmentSize & (kSegmentSize - 1)) == 0, "Power of 2!");
    static_assert((kPageSize & (kPageSize - 1)) == 0, "Power of 2!");

    const size_t kAllocationSize{[] {
      const auto page_size = (size_t)sysconf(_SC_PAGE_SIZE);
      if (page_size == size_t(-1)) {
        throw std::runtime_error("System page size not detected.");
      }
      if ((page_size & (kSegmentSize - 1)) != 0ull) {
        throw std::runtime_error("Page size must be multiple of segment size.");
      }
      return math::roundUpRuntime(kPageSize, page_size);
    }()};

    size_t allocate(size_t size) {
      const auto allocation_size = math::roundUp<kAlignment>(size);
      const auto bits_len = bitsPackLenFromSize(allocation_size);

      size_t remains;
      const auto position = searchContiguousBitPack(bits_len, remains);
      if (remains != 0ull) {
        storageAdjust(remains * kAlignment);
      }

      //markUsed()
      return position;
    }

    std::optional<size_t> deallocate(size_t ptr) {
      return std::nullopt;
    }

    MemoryAllocatorNew(size_t preallocated = 1'073'741'824ull) {
      storageAdjust(preallocated);
    }

    ~MemoryAllocatorNew() {
      free(storage_);
    }

#ifndef TEST_MODE
  private:
#endif//TEST_MODE
    auto bitsPackLenFromSize(size_t size) {
      return size / kAlignment;
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

      const auto *segment = begin;
      uint64_t segment_filter = std::numeric_limits<uint64_t>::max();
      remains = count;
      size_t position;
      do {
        const auto preprocessed_segment = (*segment & segment_filter);
        position = preprocessed_segment ? kSegmentInBits - BSF(preprocessed_segment) - 1ull : size_t(-1);
        if (position != size_t(-1)) {
          const auto leading_mask = getLeadingMask(position, count == remains);
          const auto ending_mask = getEndingMask(position, remains, count == remains);
          const auto segment_mask = (leading_mask & ending_mask);

          if ((preprocessed_segment & segment_mask) == segment_mask) {
            remains -= std::min(position + size_t(1ull), remains);
          } else {
            remains = count;
            updateSegmentFilter(segment_filter, position);
            continue;  
          }
        } else {
          remains = count;
        }
        ++segment;
        segment_filter = std::numeric_limits<uint64_t>::max();
      } while(remains > 0 && end != segment);
      return (segment - begin) * kSegmentInBits - position - 1ull;
    }

    size_t end() const {
      return table_.size() * kSegmentInBits;
    }

    template<size_t kValue>
    void setContiguousBits(size_t position, size_t count) {
      const auto segment_ix = position / kSegmentInBits;

      auto *segment = &table_[segment_ix];
      while (count > 0ull) {
        const auto shr_in_segment = position % kSegmentInBits;//kSegmentInBits - (position % kSegmentInBits) - 1ull;
        const auto shl_in_segment = ((shr_in_segment + count) >= kSegmentInBits) ? 0ull : kSegmentInBits - shr_in_segment - count;
        const auto mask = (std::numeric_limits<uint64_t>::max() >> shr_in_segment) & (std::numeric_limits<uint64_t>::max() >> shl_in_segment);

        if constexpr (kValue == 0ull) {
          *segment &= ~mask;
        } else {
          *segment |= mask;
        }

        ++segment;
        count -= std::min(count, size_t(kSegmentInBits - shr_in_segment - 1ull));
        position = 0ull;
      }
    }

    void updateSegmentFilter(uint64_t &filter, const size_t position) const {
      filter &= ~((1ull << position) | ((1ull << position) >> 1ull));
    }

    uint64_t getLeadingMask(const size_t position, bool is_starting) const {
      if (is_starting) {
        return ((1ull << position) - 1ull) | (1ull << position);
      }
      return std::numeric_limits<uint64_t>::max();
    }

    uint64_t getEndingMask(const size_t position, const size_t count, bool is_starting) const {
      const auto marker_position = is_starting ? position + 1ull - count : kSegmentInBits - count;//kSegmentInBits - (count + kSegmentInBits - position - 1ull);
      if (marker_position <= kSegmentInBits) {
        return ~((1ull << marker_position) - 1ull);
      }
      return std::numeric_limits<uint64_t>::max();
    }

    void storageAdjust(size_t size) {
      const auto was_allocated = segmentsToSize(table_.size());
      const auto added_size = math::roundUpRuntime(size, kAllocationSize);
      const auto new_size = added_size + was_allocated;

      if (__builtin_expect(new_size <= Memory::kMaxMemorySize, 1)) {
        storage_ = (uint8_t*)std::realloc(storage_, new_size);
        table_.resize(table_.size() + sizeToSegments(added_size), std::numeric_limits<uint64_t>::max());
      }
    }

    std::vector<uint64_t> table_{};
    uint8_t *storage_ = nullptr;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP
