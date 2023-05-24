/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP
#define KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP

#include <algorithm>
#include <map>
#include <cstring>
#include <memory>
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

  template <typename T, typename>
  struct Type {
    T t;

    template <typename... Args>
    explicit Type(Args &&...args) : t(std::forward<Args>(args)...) {}

    operator T &() {
      return t;
    }
  };

  using WsmPtr = Type<size_t, struct Pointer>;
  using WsmRawPtr = Type<size_t, struct RawPointer>;
  using WsmSize = Type<size_t, struct Size>;

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

      setContiguousBits<0ull>(position, bits_len);
      fixupCursor();

      auto *const header_ptr =
          (AllocationHeader *)(toAddr(position * kGranularity));
      header_ptr->count = bits_len;
      return position * kGranularity + AllocationHeader::kHeaderSize;
    }

    std::optional<size_t> deallocate(size_t offset) {
      if (offset == 0ull) {
        return std::nullopt;
      }

      const auto alloc_begin = offset - AllocationHeader::kHeaderSize;
      auto *const header_ptr = (AllocationHeader *)(toAddr(alloc_begin));

      assert((alloc_begin % kGranularity) == 0ull);
      const auto position = alloc_begin / kGranularity;

      const auto first_modified_segment =
          setContiguousBits<1ull>(position, header_ptr->count);
      cursor_ = std::min(cursor_, first_modified_segment);
      return header_ptr->count * kGranularity - AllocationHeader::kHeaderSize;
    }

    size_t realloc(size_t offset, size_t size) {
      if (size > kSegmentSize) {
        return 0ull;
      }

      if (offset == 0ull) {
        return allocate(size);
      }

      const auto alloc_begin = offset - AllocationHeader::kHeaderSize;
      auto *const header_ptr = (AllocationHeader *)(toAddr(alloc_begin));

      const auto old_bits_pack = header_ptr->count;
      const auto bits_len = bitsPackLenFromSize(
          math::roundUp<kGranularity>(size + AllocationHeader::kHeaderSize));

      if (old_bits_pack >= bits_len) {
        return offset;
      }

      const auto position = alloc_begin / kGranularity;
      const auto segment_ix = position / kSegmentInBits;
      const auto bit_ix = position % kSegmentInBits;

      uint64_t old_segment_mask_0, old_segment_mask_1;
      getFullSegmentMask(
          old_segment_mask_0, old_segment_mask_1, bit_ix, header_ptr->count);

      uint64_t new_segment_mask_0, new_segment_mask_1;
      getFullSegmentMask(
          new_segment_mask_0, new_segment_mask_1, bit_ix, bits_len);

      new_segment_mask_0 &= ~old_segment_mask_0;
      new_segment_mask_1 &= ~old_segment_mask_1;

      if (new_segment_mask_1 != 0ull && table_.size() <= segment_ix + 1) {
        storageAdjust(1ull);
      }

      if (segmentContainsPack(table_[segment_ix], new_segment_mask_0)
          && (new_segment_mask_1 == 0ull
              || segmentContainsPack(table_[segment_ix + 1],
                                     new_segment_mask_1))) {
        table_[segment_ix] &= ~new_segment_mask_0;
        table_[segment_ix + 1] &= ~new_segment_mask_1;
        fixupCursor();

        header_ptr->count = bits_len;
        return offset;
      }

      const auto new_offset = allocate(size);
      memcpy(
          toAddr(new_offset),
          toAddr(offset),
          sizeFromBitsPackLen(old_bits_pack) - AllocationHeader::kHeaderSize);
      deallocate(offset);
      return new_offset;
    }

    uint8_t *toAddr(uint64_t offset) {
      return startAddr() + offset;
    }

    const uint8_t *toAddr(uint64_t offset) const {
      return startAddr() + offset;
    }

    size_t capacity() const {
      return table_.size() * kSegmentSize;
    }

    size_t size(size_t offset) const {
      const auto alloc_begin = offset - AllocationHeader::kHeaderSize;
      const auto *const header_ptr = (AllocationHeader *)(toAddr(alloc_begin));
      return (header_ptr->count * kGranularity - AllocationHeader::kHeaderSize);
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

    auto sizeFromBitsPackLen(size_t count) {
      return count * kGranularity;
    }

    auto segmentsToSize(size_t val) {
      return val * kSegmentSize;
    }

    void fixupCursor() {
      while (cursor_ < table_.size() && table_[cursor_] == 0ull) {
        ++cursor_;
      }
    }

    void getFullSegmentMask(uint64_t &segment_0,
                            uint64_t &segment_1,
                            size_t position,
                            size_t count) const {
      size_t remains;
      getFullSegmentMask(segment_0, segment_1, position, count, remains);
    }

    void getFullSegmentMask(uint64_t &segment_0,
                            uint64_t &segment_1,
                            size_t position,
                            size_t count,
                            size_t &remains) const {
      segment_0 = getSegmentMask<true>(position, count, remains);
      segment_1 = getSegmentMask<false>(0ull, remains, remains);
    }

    const AllocationHeader &getHeader(uint64_t offset) {
      return *(const AllocationHeader *)toAddr(offset
                                               - AllocationHeader::kHeaderSize);
    }

    bool segmentContainsPack(uint64_t segment, uint64_t pack) const {
      return (segment & pack) == pack;
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
      remains = count;
      if (table_.empty()) {
        return 0ull;
      }

      const auto *const begin = table_.data();
      const auto *const end = table_.data() + table_.size();

      const auto *segment = &table_[cursor_];
      uint64_t preprocessed_segment_filter =
          std::numeric_limits<uint64_t>::max();

      size_t position = 0ull;
      while (end != segment) {
        remains = count;
        const auto preprocessed_segment =
            (*segment & preprocessed_segment_filter);
        position = (preprocessed_segment == 0ull ? kSegmentInBits
                                                 : BSR(preprocessed_segment));

        if (__builtin_expect(position != kSegmentInBits, 1)) {
          uint64_t segment_mask_0, segment_mask_1;
          getFullSegmentMask(
              segment_mask_0, segment_mask_1, position, count, remains);

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

    uint8_t *startAddr() {
      return storage_;
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
            getSegmentMask<false>(0ull, remains, remains);
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
      const auto added_size = math::roundUp<kAllocationSize>(size);
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

  struct GenericAllocator {
    GenericAllocator(size_t heap_base) : heap_base_{heap_base} {}

    WsmPtr allocate(WsmSize size) {
      if ((size - 1ull) / std::get<0>(layers_).kSegmentSize == 0ull) {
        return WsmPtr(std::get<0>(layers_).allocate(size) + heap_base_);
      }

      if ((size - 1ull) / std::get<1>(layers_).kSegmentSize == 0ull) {
        return WsmPtr(std::get<1>(layers_).allocate(size) + heap_base_);
      }

      if ((size - 1ull) / std::get<2>(layers_).kSegmentSize == 0ull) {
        return WsmPtr(std::get<2>(layers_).allocate(size) + heap_base_);
      }

      if ((size - 1ull) / std::get<3>(layers_).kSegmentSize == 0ull) {
        return WsmPtr(std::get<3>(layers_).allocate(size) + heap_base_);
      }

      assert(false && "No layer for current allocation!");
      return WsmPtr(0ull);
    }

    std::optional<WsmSize> deallocate(WsmPtr offset) {
      const auto raw_offset = WsmRawPtr(offset - heap_base_);
      std::optional<WsmSize> result;
      for_each_layer([&](auto &layer) {
        if (!layer.offsetLocatesHere(raw_offset)) {
          return;
        }

        assert(!result);
        result = layer.deallocate(raw_offset);
      });
      return result;
    }

    WsmPtr realloc(WsmPtr offset, WsmSize size) {
      if (offset == 0ull) {
        return allocate(size);
      }

      if (auto position = tryReallocInLayer(offset, size); position != 0ull) {
        return position;
      }

      const auto position = allocate(size);
      auto *dst = toPtr(position);
      auto *src = toPtr(offset);
      const auto src_sz = allocationSize(offset);

      memcpy(dst, src, src_sz.t);
      deallocate(offset);
      return position;
    }

    WsmSize allocationSize(WsmPtr offset) {
      const auto raw_offset = WsmRawPtr(offset - heap_base_);
      WsmSize result{0ull};
      for_each_layer([&](auto &layer) {
        if (!layer.offsetLocatesHere(raw_offset)) {
          return;
        }

        assert(result.t == 0ull);
        result = layer.size(raw_offset);
      });
      return result;
    }

    uint8_t *toPtr(WsmPtr offset) {
      const auto raw_offset = WsmRawPtr(offset - heap_base_);
      uint8_t *result = nullptr;
      for_each_layer([&](auto &layer) {
        if (!layer.offsetLocatesHere(raw_offset)) {
          return;
        }

        assert(result == nullptr);
        result = layer.toAddr(raw_offset);
      });
      return result;
    }

#ifndef TEST_MODE
   private:
#endif  // TEST_MODE
    template <typename F>
    void for_each_layer(F &&func) {
      return std::apply([&](auto &...value) { (..., (func(value))); }, layers_);
    }

    WsmPtr tryReallocInLayer(WsmPtr offset, WsmSize size) {
      const auto raw_offset = WsmRawPtr(offset - heap_base_);
      WsmRawPtr result;
      for_each_layer([&](auto &layer) {
        if (!layer.offsetLocatesHere(raw_offset)) {
          return;
        }

        assert(!result);
        result = layer.realloc(raw_offset, size);
      });
      return result != 0ull ? WsmPtr(result + heap_base_) : WsmPtr(0ull);
    }

    template <size_t Granularity, size_t AddressOffset>
    struct Layer final {
      static constexpr auto kAddressOffset = AddressOffset;
      static constexpr auto kGranularity = Granularity;
      static constexpr auto kSegmentSize =
          MemoryAllocatorNew<Granularity>::kSegmentSize;
      explicit Layer(size_t preallocated = 0ull) : bank_{preallocated} {}

      bool offsetLocatesHere(WsmRawPtr offset) const {
        return ((offset - AddressOffset) < bank_.capacity());
      }

      WsmRawPtr allocate(WsmSize size) {
        return WsmRawPtr(bank_.allocate(size) + AddressOffset);
      }

      std::optional<WsmSize> deallocate(WsmRawPtr offset) {
        if (auto result = bank_.deallocate(offset - AddressOffset)) {
          return WsmSize(*result);
        }
        return std::nullopt;
      }

      WsmRawPtr realloc(WsmRawPtr offset, WsmSize size) {
        const auto ptr = bank_.realloc(offset - AddressOffset, size);
        return ptr != 0ull ? WsmRawPtr(ptr + AddressOffset) : WsmRawPtr(0ull);
      }

      uint8_t *toAddr(WsmRawPtr offset) {
        return bank_.toAddr(offset - AddressOffset);
      }

      WsmSize size(WsmRawPtr offset) const {
        return WsmSize(bank_.size(offset - AddressOffset));
      }

#ifndef TEST_MODE
     private:
#endif  // TEST_MODE
      MemoryAllocatorNew<Granularity> bank_;
    };

    const size_t heap_base_;
    std::tuple<Layer<8ull, 0ull>,
               Layer<8ull * 64ull, 1ull * 1024ull * 1024ull * 1024ull>,
               Layer<8ull * 64ull * 64ull, 2ull * 1024ull * 1024ull * 1024ull>,
               Layer<8ull * 64ull * 64ull * 64ull,
                     3ull * 1024ull * 1024ull * 1024ull> >
        layers_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_COMMON_MEMORY_ALLOCATOR_HPP
