/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/buffer_view.hpp"

#include <gtest/gtest.h>

using namespace kagome::common;
using Span = gsl::span<const uint8_t>;

TEST(BufferView, Constructor_default) {
  BufferView v;

  EXPECT_EQ(v.toHex(), "");
  EXPECT_EQ(v.size(), 0);
}

TEST(BufferView, Constructor_from_span) {
  uint8_t c_arr[] = {1, 2, 3, 4, 5};
  Span span(std::begin(c_arr), std::end(c_arr));

  BufferView view_span(span);

  EXPECT_EQ(view_span.toHex(), "0102030405");
  EXPECT_EQ(view_span.size(), std::size(c_arr));
}

TEST(BufferView, Constructor_from_vector) {
  std::vector<uint8_t> vec = {1, 2, 3, 4, 5};

  BufferView view_vec(vec);

  EXPECT_EQ(view_vec.toHex(), "0102030405");
  EXPECT_EQ(view_vec.size(), vec.size());
}

TEST(BufferView, Constructor_from_array) {
  std::array<uint8_t, 5> arr = {1, 2, 3, 4, 5};

  BufferView view_arr(arr);

  EXPECT_EQ(view_arr.toHex(), "0102030405");
  EXPECT_EQ(view_arr.size(), arr.size());
}

TEST(BufferView, Constructor_from_BufferView) {
  std::array<uint8_t, 5> arr = {1, 2, 3, 4, 5};
  BufferView view_arr(arr);

  BufferView view_view(view_arr);

  EXPECT_EQ(view_view.toHex(), "0102030405");
  EXPECT_EQ(view_view.size(), arr.size());
}
