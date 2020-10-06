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
    std::vector<EncodeOpaqueValue> vec_inp1(
        {EncodeOpaqueValue{scale::encode(inp1).value()}});

    std::vector<uint8_t> res{};
    auto encoded_concat_res = append_or_new_vec(res, vec_inp1);

    ASSERT_THAT(
        encoded_concat_res.value(),
        ContainerEq(std::vector<uint8_t>({4, 20, 1, 0, 0, 0, 2, 0, 0, 0, 3,
                                          0, 0,  0, 4, 0, 0, 0, 5, 0, 0, 0})));

    std::vector<EncodeOpaqueValue> vec_inp2(
        {EncodeOpaqueValue{scale::encode(uint32_t{2}).value()}});
    encoded_concat_res = append_or_new_vec(res, vec_inp2);

    ASSERT_THAT(encoded_concat_res.value(),
                ContainerEq(std::vector<uint8_t>({8, 20, 1, 0, 0, 0, 2, 0, 0,
                                                  0, 3,  0, 0, 0, 4, 0, 0, 0,
                                                  5, 0,  0, 0, 2, 0, 0, 0})));
  }
}  // namespace kagome::scale
