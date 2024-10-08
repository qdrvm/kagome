/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <type_traits>
#include "primitives/math.hpp"
#include "pvm/types.hpp"

namespace kagome::pvm {

  struct Cursor {
    Cursor(std::span<const uint8_t> input) : input_(input), ptr_(&input_[0]) {}

    template <typename T>
      requires std::is_arithmetic_v<T>
    Result<T> read() {
      if (ptr_ + sizeof(T) > &input_[input_.size()]) {
        return Error::NOT_ENOUGH_DATA;
      }

      const auto result = *(T *)ptr_;
      ptr_ += sizeof(T);
      return result;
    }

    template <typename T>
      requires std::is_arithmetic_v<T>
    Result<const T *const> read(uint32_t count) {
      if (ptr_ + count * sizeof(T) > &input_[input_.size()]) {
        return Error::NOT_ENOUGH_DATA;
      }

      const T *const result = (const T *const)ptr_;
      ptr_ += sizeof(T) * count;
      return result;
    }

    Result<uint32_t> read_varint() {
      OUTCOME_TRY(first_byte, read<uint8_t>());
      const uint32_t length =
          first_byte == 0xff ? 8 : __builtin_clz(uint8_t(~first_byte)) - 24;
      const uint32_t upper_mask = 0x000000ff >> length;
      const uint32_t upper_bits =
          math::wrapped_shl(upper_mask & uint32_t(first_byte), length * 8);

      OUTCOME_TRY(input, read<uint8_t>(length));
      switch (length) {
        case 0:
          return upper_bits;
        case 1:
          return (upper_bits | uint32_t(input[0]));
        case 2:
          return (upper_bits | math::fromLE(*(uint16_t *)input));
        case 3: {
          uint8_t buf[] = {input[0], input[1], input[2], 0};
          return (upper_bits | math::fromLE(*(uint32_t *)buf));
        }
        case 4:
          return (upper_bits | math::fromLE(*(uint32_t *)input));
        default:
          return Error::FAILED_TO_READ_UVARINT;
      }
    }

    uintptr_t get_offset() const {
      return ptr_ - &input_[0];
    }

    size_t get_len() const {
      return input_.size();
    }

    std::span<const uint8_t> get_section() const {
      return input_;
    }

   private:
    std::span<const uint8_t> input_;
    const uint8_t *ptr_;
  };

}  // namespace kagome::pvm
