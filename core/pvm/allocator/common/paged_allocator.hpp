/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <concepts>
#include <deque>
#include <type_traits>

#include "pvm/native/linux.hpp"
#include "pvm/types.hpp"

namespace kagome::pvm {

  template <typename T>
  concept InitialZero = requires {
    { T::zero() } -> std::same_as<T>;
  };

  template <typename T>
  concept EmptyDefault = requires {
    { T::empty_default() } -> std::same_as<T>;
  };

  template <typename T>
  concept Indexed = requires(T t, uint32_t ix) {
    typename T::Index;
    { t.index(ix) } -> std::convertible_to<typename T::Index>;
  };

  template <typename T>
  concept AllocatorCfg = requires {
    typename T::Size;
    typename T::BitMask;
    typename T::BinArray;

    { T::template to_bin_index<true>() } -> std::convertible_to<uint32_t>;
    { T::template to_bin_index<false>() } -> std::convertible_to<uint32_t>;

    requires std::is_same_v<typename T::MAX_ALLOCATION_SIZE, typename T::Size>;
    requires InitialZero<typename T::Size>;
    requires EmptyDefault<typename T::BitMask>;
    requires EmptyDefault<typename T::BinArray>;
    requires Indexed<typename T::BitMask>;
  };

  template <typename C>
    requires AllocatorCfg<C>
  class GenericAllocator {
    using Config = C;
    using Size = Config::Size;
    using BitMask = Config::BitMask;
    using BinArray = Config::BinArray;

    struct Node {
      uint32_t next_by_address;
      uint32_t prev_by_address;
      uint32_t next_in_bin;
      uint32_t prev_in_bin;
      Size offset;
      Size size;
      bool is_allocated;
    };

    class GenericAllocation {
      uint32_t node_;
      Size offset_;
      Size size_;

     public:
      static constexpr uint32_t EMPTY = std::numeric_limits<uint32_t>::max();
      static constexpr GenericAllocation DEFAULT = GenericAllocation{
          .node = EMPTY,
          .offset = Size::ZERO,
          .size = Size::ZERO,
      };

      bool is_empty() const;
      Size offset() const;
      Size size() const;
    };

    std::deque<Node> nodes;
    std::deque<uint32_t> unused_node_slots;
    BitMask bins_with_free_space;
    BinArray first_unallocated_for_bin;

   public:
    GenericAllocator(Size total);

    static BitMask::Index size_to_bin_round_down(
        GenericAllocator<C>::Size size);
    static BitMask::Index size_to_bin_round_up(GenericAllocator<C>::Size size);
  };

  template <typename C>
    requires AllocatorCfg<C>
  bool GenericAllocator<C>::GenericAllocation::is_empty() const {
    return EMPTY == node_;
  }

  template <typename C>
    requires AllocatorCfg<C>
  GenericAllocator<C>::Size GenericAllocator<C>::GenericAllocation::offset()
      const {
    return offset_;
  }

  template <typename C>
    requires AllocatorCfg<C>
  GenericAllocator<C>::Size GenericAllocator<C>::GenericAllocation::size()
      const {
    return size_;
  }

  template <typename C>
    requires AllocatorCfg<C>
  GenericAllocator<C>::GenericAllocator(GenericAllocator<C>::Size total_space)
      : bins_with_free_space{GenericAllocator<C>::BitMask::empty_default()},
        first_unallocated_for_bin{
            GenericAllocator<C>::BinArray::empty_default()} {
    insert_free_node(0, total_space);
  }

  template <typename C>
    requires AllocatorCfg<C>
  GenericAllocator<C>::BitMask::Index
  GenericAllocator<C>::size_to_bin_round_down(GenericAllocator<C>::Size size) {
    const auto sz = std::min(size, Config::MAX_ALLOCATION_SIZE);
    return BitMask::index(Config::to_bin_index<false>(sz));
  }

  template <typename C>
    requires AllocatorCfg<C>
  GenericAllocator<C>::BitMask::Index GenericAllocator<C>::size_to_bin_round_up(
      GenericAllocator<C>::Size size) {
    const auto sz = std::min(size, Config::MAX_ALLOCATION_SIZE);
    return BitMask::index(Config::to_bin_index<true>(sz));
  }

}  // namespace kagome::pvm
