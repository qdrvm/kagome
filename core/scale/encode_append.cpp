/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/encode_append.hpp"

#include "scale/compact_len_utils.hpp"

namespace kagome::scale {

  /**
   * @param data Scale encoded vector of EncodeOpaqueValues
   * @return tuple containing:
   *  1. new length of vector after inserting there one more EncodeOpaqueValues
   *  2. current length of CompactInteger-scale-encoded length of
   * EncodeOpaqueValues vector
   *  3. length of CompactInteger-scale-encoded length of EncodeOpaqueValues
   * vector after insertion there one more EncodeOpaqueValue
   */
  outcome::result<std::tuple<uint32_t, uint32_t, uint32_t>> extract_length_data(
      const std::vector<uint8_t> &data) {
    OUTCOME_TRY(len, scale::decode<CompactInteger>(data));
    auto new_len = (len + 1).convert_to<uint32_t>();
    auto encoded_len = compact::compactLen(len.convert_to<uint32_t>());
    auto encoded_new_len = compact::compactLen(new_len);
    return std::make_tuple(new_len, encoded_len, encoded_new_len);
  }

  outcome::result<void> append_or_new_vec(std::vector<uint8_t> &self_encoded,
                                          gsl::span<const uint8_t> input) {
    EncodeOpaqueValue opaque_value{.v = input};

    // No data present, just encode the given input data.
    if (self_encoded.empty()) {
      self_encoded =
          scale::encode(std::vector<EncodeOpaqueValue>{opaque_value}).value();
      return outcome::success();
    }

    OUTCOME_TRY(extract_tuple, extract_length_data(self_encoded));
    const auto &[new_len, encoded_len, encoded_new_len] = extract_tuple;

    auto replace_len = [new_len = new_len](std::vector<uint8_t> &dest) {
      auto e = scale::encode(CompactInteger{new_len}).value();
      std::move(e.begin(), e.end(), dest.begin());
    };

    // If old and new encoded len is equal, we don't need to copy the
    // already encoded data.
    if (encoded_len != encoded_new_len) {
      // reserve place for new len, old vector and new vector
      self_encoded.reserve(encoded_new_len + (self_encoded.size() - encoded_len)
                           + opaque_value.v.size());

      // shift encoded new len by one element to give space for new Compact
      // encoded length
      const auto shift_size = 1;  // encoded_new_len - encoded_len is always 1
      BOOST_ASSERT(encoded_new_len - encoded_len == shift_size);
      self_encoded.resize(self_encoded.size() + shift_size);
      std::rotate(self_encoded.rbegin(),
                  self_encoded.rbegin() + 1,
                  self_encoded.rend());
    } else {
      // reserve place for existing and new vector
      self_encoded.reserve(self_encoded.size() + opaque_value.v.size());
    }
    replace_len(self_encoded);
    self_encoded.insert(
        self_encoded.end(), opaque_value.v.begin(), opaque_value.v.end());
    return outcome::success();
  }
}  // namespace kagome::scale
