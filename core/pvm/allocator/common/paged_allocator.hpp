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
  concept IndexedMask = requires(T t, uint32_t ix, typename T::Index idx) {
    typename T::Index;

    { t.index(ix) } -> std::convertible_to<typename T::Index>;
    { t.find_first(idx) } -> std::same_as<Opt<typename T::Index>>;

    { t.set(idx) };
    { t.unset(idx) };
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
    requires IndexedMask<typename T::BitMask>;

    requires EmptyDefault<typename T::BitMask>;
    requires EmptyDefault<typename T::BinArray>;
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

    std::deque<Node> nodes;
    std::deque<uint32_t> unused_node_slots;
    BitMask bins_with_free_space;
    BinArray first_unallocated_for_bin;

    static BitMask::Index size_to_bin_round_down(Size size);
    static BitMask::Index size_to_bin_round_up(Size size);

    Opt<uint32_t> insert_free_node(Size offset, Size size);
    void remove_node(uint32_t node);
    void remove_first_free_node(uint32_t node, BitMask::Index bin);

   public:
    struct GenericAllocation {
      uint32_t node_;
      Size offset_;
      Size size_;

      static constexpr uint32_t EMPTY = std::numeric_limits<uint32_t>::max();
      static constexpr GenericAllocation DEFAULT = GenericAllocation{
          .node_ = EMPTY,
          .offset_ = Size::ZERO,
          .size_ = Size::ZERO,
      };

      bool is_empty() const;
      Size offset() const;
      Size size() const;
    };

    GenericAllocator(Size total);
    Opt<GenericAllocation> alloc(Size size);
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

  template <typename C>
    requires AllocatorCfg<C>
  Opt<uint32_t> GenericAllocator<C>::insert_free_node(
      GenericAllocator<C>::Size offset, GenericAllocator<C>::Size size) {
    if (0 == size) {
      return std::nullopt;
    }

    const auto bin = size_to_bin_round_down(size);
    const auto first_node_in_bin = first_unallocated_for_bin[bin.index()];
    const Node region{
        .next_by_address = GenericAllocation::EMPTY,
        .prev_by_address = GenericAllocation::EMPTY,
        .next_in_bin = first_node_in_bin,
        .prev_in_bin = GenericAllocation::EMPTY,
        .offset = offset,
        .size = size,
        .is_allocated = false,
    };

    uint32_t new_node;
    if (!unused_node_slots.empty()) {
      const auto nn = unused_node_slots.back();
      unused_node_slots.pop_back();
      nodes[nn] = region;
      new_node = nn;
    } else {
      const auto nn = uint32_t(nodes.size());
      nodes.push_back(region);
      new_node = nn;
    }

    if (first_node_in_bin < nodes.size()) {
      auto &node = nodes[first_node_in_bin];
      node.prev_in_bin = new_node;
    } else {
      bins_with_free_space.set(bin);
    }

    first_unallocated_for_bin[bin.index()] = new_node;
    return {new_node};
  }

  template <typename C>
    requires AllocatorCfg<C>
  void GenericAllocator<C>::remove_node(uint32_t node) {
    const auto prev_in_bin = nodes[node].prev_in_bin;
    if (GenericAllocation::EMPTY != prev_in_bin) {
      const auto next_in_bin = nodes[node].next_in_bin;
      nodes[prev_in_bin].next_in_bin = next_in_bin;

      if (next_in_bin < nodes.size()) {
        nodes[next_in_bin].prev_in_bin = prev_in_bin;
      } else {
        assert(GenericAllocation::EMPTY == next_in_bin);
      }
    } else {
      const auto bin = size_to_bin_round_down(nodes[node].size);
      remove_first_free_node(node, bin);
    }
    unused_node_slots.push_back(node);
  }

  template <typename C>
    requires AllocatorCfg<C>
  void GenericAllocator<C>::remove_first_free_node(
      uint32_t node, GenericAllocator<C>::BitMask::Index bin) {
    assert(first_unallocated_for_bin[bin.index()] == node);
    const auto next_in_bin = nodes[node].next_in_bin;
    first_unallocated_for_bin[bin.index()] = next_in_bin;

    if (next_in_bin < nodes.size()) {
      nodes[next_in_bin].prev_in_bin = GenericAllocation::EMPTY;
    } else {
      assert(GenericAllocation::EMPTY == next_in_bin);
      bins_with_free_space.unset(bin);
    }
  }

      template <typename C>
    requires AllocatorCfg<C>
  Opt<typename GenericAllocator<C>::GenericAllocation> GenericAllocator<C>::alloc(GenericAllocator<C>::Size size) {
        if (0 == size) {
            return {GenericAllocation::DEFAULT};
        }

        if (size > Config::MAX_ALLOCATION_SIZE) {
            return std::nullopt;
        }

        typename BitMask::Index bin;
        uint32_t node;
        if (const auto opt_bin = bins_with_free_space.find_first(size_to_bin_round_up(size))) {
            bin = *opt_bin;
            node = first_unallocated_for_bin[opt_bin->index()];
        }
        if (const auto opt_bin = bins_with_free_space.find_first(size_to_bin_round_down(size))) {
            const auto n = first_unallocated_for_bin[opt_bin->index()];
            if (nodes[node].size < size) {
                return std::nullopt;
            }
            bin = *opt_bin;
            node = n;
        } else {
            return std::nullopt;
        }

        const auto original_size = nodes[node].size;
        nodes[node].size = size;

        assert(original_size >= size);
        assert(!nodes[node].is_allocated);
        nodes[node].is_allocated = true;

        remove_first_free_node(node, bin);

        const auto offset = nodes[node].offset;
        const auto remaining_free_pages = original_size - size;

        if (const auto new_free_node = insert_free_node(offset + size, remaining_free_pages)) {
            const auto next_by_address = nodes[node].next_by_address;
            nodes[node].next_by_address = new_free_node;

            if (next_by_address < nodes.size()) {
              nodes[next_by_address].prev_by_address = new_free_node;
            } else {
              assert(GenericAllocation::EMPTY == next_by_address);
            }
            nodes[new_free_node].prev_by_address = node;
            nodes[new_free_node].next_by_address = next_by_address;
        }

        return {GenericAllocation { 
          .node_ = node, 
          .offset_ = offset, 
          .size_ = size,
        }};
    }

}  // namespace kagome::pvm
