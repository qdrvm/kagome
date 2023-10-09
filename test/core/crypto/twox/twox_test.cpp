/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/twox/twox.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "testutil/literals.hpp"

using kagome::common::Buffer;
using namespace kagome::crypto;

/**
 * @given some strings
 * @when calling make_twox128
 * @then resulting hash matches to the magic
 */
TEST(Twox128, Correctness) {
  {
    auto hash = make_twox128({});
    // clang-format off
    uint8_t reference[16] = {153, 233, 216, 81, 55, 219, 70, 239, 75, 190, 163, 54, 19, 186, 175, 213};
    // clang-format on
    ASSERT_THAT(hash, ::testing::ElementsAreArray(reference));
  }
  {
    auto hash = make_twox128("414243444546"_unhex);
    // clang-format off
    uint8_t reference[16] = {184, 65, 176, 250, 243, 129, 181, 3, 77, 82, 63, 150, 129, 221, 191, 251};
    // clang-format on
    ASSERT_THAT(hash, ::testing::ElementsAreArray(reference));
  }
}

/**
 * @given some strings
 * @when calling make_twox256
 * @then resulting hash matches to the magic
 */
TEST(Twox256, Correctness) {
  {
    auto hash = make_twox256({});
    // clang-format off
    uint8_t reference[32] = {153, 233, 216, 81, 55, 219, 70, 239, 75, 190, 163, 54, 19, 186, 175, 213, 111, 150, 60, 100, 177, 243, 104, 90, 78, 180, 171, 214, 127, 246, 32, 58};
    // clang-format on
    ASSERT_THAT(hash, ::testing::ElementsAreArray(reference));
  }
  {
    auto hash = make_twox256("414243444546"_unhex);
    // clang-format off
    uint8_t reference[32] = {184, 65, 176, 250, 243, 129, 181, 3, 77, 82, 63, 150, 129, 221, 191, 251, 33, 226, 149, 136, 6, 232, 81, 118, 200, 28, 69, 219, 120, 179, 208, 237};
    // clang-format on
    ASSERT_THAT(hash, ::testing::ElementsAreArray(reference));
  }
}
