/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SCALE_ENCODE_APPEND_HPP
#define KAGOME_CORE_SCALE_ENCODE_APPEND_HPP

#include "scale/compact_len_utils.hpp"
#include "scale/scale.hpp"

namespace kagome::scale {

  outcome::result<std::tuple<uint32_t, uint32_t, uint32_t>> extract_length_data(
      const std::vector<uint8_t> &data, size_t input_len) {
    OUTCOME_TRY(len, scale::decode<CompactInteger>(data));
    auto new_len = (len + input_len).convert_to<uint32_t>();
    auto encoded_len = compact::compactLen(len.convert_to<uint32_t>());
    auto encoded_new_len = compact::compactLen(new_len);
    return std::make_tuple(new_len, encoded_len, encoded_new_len);
  }

  outcome::result<std::vector<uint8_t>> append_or_new_vec_with_any_item(
      std::vector<uint8_t> &self_encoded, const std::vector<uint8_t> &input) {
    auto input_len = input.size();

    // No data present, just encode the given input data.
    if (self_encoded.empty()) {
      return scale::encode(input).value();
    }

    OUTCOME_TRY(extract_tuple, extract_length_data(self_encoded, input_len));
    const auto &[new_len, encoded_len, encoded_new_len] = extract_tuple;

    auto replace_len = [new_len = new_len, encoded_new_len = encoded_new_len](
                           std::vector<uint8_t> &dest) {
      auto e = scale::encode(CompactInteger{new_len}).value();
      BOOST_ASSERT(encoded_new_len == e.size());
      std::copy(e.begin(), e.end(), dest.begin());
    };

    // If old and new encoded len is equal, we don't need to copy the
    // already encoded data.
    if (encoded_len == encoded_new_len) {
      replace_len(self_encoded);
      self_encoded.reserve(self_encoded.size() + input.size());
      self_encoded.insert(self_encoded.end(), input.begin(), input.end());
      return self_encoded;
    }
    auto size = encoded_new_len + self_encoded.size() - encoded_len;

    std::vector<uint8_t> res{};
    res.reserve(size + input_len);
    res.resize(size);

    // Insert the new encoded len, copy the already encoded data and
    // add the new element.
    replace_len(res);
    res.insert(res.begin() + encoded_new_len,
               self_encoded.begin() + encoded_len,
               self_encoded.end());
    res.insert(res.end(), input.begin(), input.end());
    return res;
  }
}  // namespace kagome::scale

#endif  // KAGOME_CORE_SCALE_ENCODE_APPEND_HPP
