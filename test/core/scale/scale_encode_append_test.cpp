/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/encode_append.hpp"

#include <gtest/gtest.h>

namespace kagome::scale {

  TEST(EncodeAppend, AppendVecTest) {
    std::vector<uint8_t> a{1, 2, 3, 4, 5};
    std::vector<uint8_t> b{6, 7, 8, 9, 10};

    std::vector<uint8_t> concat{};
    concat.insert(concat.end(), a.begin(), a.end());
    concat.insert(concat.end(), b.begin(), b.end());

    auto encoded_a = scale::encode(a).value();
    // auto encoded_b = scale::encode(b).value();

    auto encoded_concat_res =
        append_or_new_vec_with_any_item(encoded_a, b);
    ASSERT_TRUE(encoded_concat_res.has_value());
    auto decoded_concat_res =
        scale::decode<std::vector<uint8_t>>(encoded_concat_res.value());
    ASSERT_TRUE(decoded_concat_res.has_value())
        << decoded_concat_res.error().message();
    ASSERT_EQ(concat, decoded_concat_res.value());
  }

}  // namespace kagome::scale
