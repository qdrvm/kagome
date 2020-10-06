/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "scale/encode_append.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::ContainerEq;
using ::testing::ElementsAre;

namespace kagome::scale {

  TEST(EncodeAppend, Append) {
    std::vector<uint32_t> inp1({1, 2, 3, 4, 5});
    EncodeOpaqueValue inp1_eov{scale::encode(inp1).value()};

    std::vector<uint8_t> res{};
    auto encoded_concat_res =
        append_or_new_vec(res, scale::encode(inp1_eov).value());

    ASSERT_TRUE(encoded_concat_res.has_value());
    ASSERT_THAT(res, ContainerEq(std::vector<uint8_t>({4, 20, 1, 0, 0, 0, 2, 0,
                                                       0, 0,  3, 0, 0, 0, 4, 0,
                                                       0, 0,  5, 0, 0, 0})));

    EncodeOpaqueValue inp2_eov{scale::encode(uint32_t{2}).value()};
    auto aa = scale::encode(inp2_eov).value();
    encoded_concat_res =
        append_or_new_vec(res, aa);

    ASSERT_TRUE(encoded_concat_res.has_value());
    ASSERT_THAT(res,
                ContainerEq(std::vector<uint8_t>({8, 20, 1, 0, 0, 0, 2, 0, 0,
                                                  0, 3,  0, 0, 0, 4, 0, 0, 0,
                                                  5, 0,  0, 0, 2, 0, 0, 0})));
  }
}  // namespace kagome::scale
