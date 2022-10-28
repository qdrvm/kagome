/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/size_limited_containers.hpp"

#include <gtest/gtest.h>

using namespace kagome::common;
using namespace std::string_literals;

TEST(SLVector, Constructor_default) {
  using Container = SLVector<int, 2>;

  ASSERT_NO_THROW(Container z);
}

TEST(SLVector, Constructors_with_size) {
  using Container = SLVector<int, 2>;

  // With size and def value
  ASSERT_NO_THROW(Container v0(0ULL));
  ASSERT_NO_THROW(Container v1(1));
  ASSERT_NO_THROW(Container v2(2));
  ASSERT_THROW(Container v3(3), MaxSizeException);

  // With size and value
  ASSERT_NO_THROW(Container v0(0, 0));
  ASSERT_NO_THROW(Container v1(1, 1));
  ASSERT_NO_THROW(Container v2(2, 2));
  ASSERT_THROW(Container v3(3, 3), MaxSizeException);
}

TEST(SLVector, Constructors_by_copy_and_movement) {
  using Container1 = SLVector<int, 1>;
  using Container2 = SLVector<int, 2>;
  using Container3 = SLVector<int, 3>;

  Container1 src_1_1(1);

  Container2 src_2_1(1);
  Container2 src_2_2(2);

  Container3 src_3_1(1);
  Container3 src_3_2(2);
  Container3 src_3_3(3);

  std::vector<int> v1(1);
  std::vector<int> v2(2);
  std::vector<int> v3(3);

  // Copy
  ASSERT_NO_THROW(Container2 dst(v1));
  ASSERT_NO_THROW(Container2 dst(v2));
  ASSERT_THROW(Container2 dst(v3), MaxSizeException);

  ASSERT_NO_THROW(Container2 dst(src_1_1));
  ASSERT_NO_THROW(Container2 dst(src_2_1));
  ASSERT_NO_THROW(Container2 dst(src_2_2));
  ASSERT_NO_THROW(Container2 dst(src_3_1));
  ASSERT_NO_THROW(Container2 dst(src_3_2));
  ASSERT_THROW(Container2 dst(src_3_3), MaxSizeException);

  // Movement
  ASSERT_NO_THROW(Container2 dst(std::move(v1)));
  ASSERT_NO_THROW(Container2 dst(std::move(v2)));
  ASSERT_THROW(Container2 dst(std::move(v3)), MaxSizeException);

  ASSERT_NO_THROW(Container2 dst(std::move(src_1_1)));
  ASSERT_NO_THROW(Container2 dst(std::move(src_2_1)));
  ASSERT_NO_THROW(Container2 dst(std::move(src_2_2)));
  ASSERT_NO_THROW(Container2 dst(std::move(src_3_1)));
  ASSERT_NO_THROW(Container2 dst(std::move(src_3_2)));
  ASSERT_THROW(Container2 dst(std::move(src_3_3)), MaxSizeException);
}

TEST(SLVector, Constructors_by_range) {
  using Container2 = SLVector<int, 2>;

  std::vector<int> v1(1);
  std::vector<int> v2(2);
  std::vector<int> v3(3);

  ASSERT_NO_THROW(Container2 dst(v1.begin(), v1.end()));
  ASSERT_NO_THROW(Container2 dst(v2.begin(), v2.end()));
  ASSERT_THROW(Container2 dst(v3.begin(), v3.end()), MaxSizeException);
}

TEST(SLVector, Constructors_by_initialier_list) {
  using Container3 = SLVector<int, 3>;

  ASSERT_NO_THROW(Container3 dst({}));
  ASSERT_NO_THROW(Container3 dst({1}));
  ASSERT_NO_THROW(Container3 dst({1, 2}));
  ASSERT_NO_THROW(Container3 dst({1, 2, 3}));
  ASSERT_THROW(Container3 dst({1, 2, 3, 4}), MaxSizeException);
}

TEST(SLVector, AssignmentOperators_by_copy_and_movement) {
  using Container1 = SLVector<int, 1>;
  using Container2 = SLVector<int, 2>;
  using Container3 = SLVector<int, 3>;

  Container1 src_1_1(1);

  Container2 src_2_1(1);
  Container2 src_2_2(2);

  Container3 src_3_1(1);
  Container3 src_3_2(2);
  Container3 src_3_3(3);

  std::vector<int> v1(1);
  std::vector<int> v2(2);
  std::vector<int> v3(3);

  Container2 dst;

  // Copy
  ASSERT_NO_THROW(dst = v1);
  ASSERT_NO_THROW(dst = v2);
  ASSERT_THROW(dst = v3, MaxSizeException);

  ASSERT_NO_THROW(dst = src_1_1);
  ASSERT_NO_THROW(dst = src_2_1);
  ASSERT_NO_THROW(dst = src_2_2);
  ASSERT_NO_THROW(dst = src_3_1);
  ASSERT_NO_THROW(dst = src_3_2);
  ASSERT_THROW(dst = src_3_3, MaxSizeException);

  // Movement
  ASSERT_NO_THROW(dst = std::move(v1));
  ASSERT_NO_THROW(dst = std::move(v2));
  ASSERT_THROW(dst = std::move(v3), MaxSizeException);

  ASSERT_NO_THROW(dst = std::move(src_1_1));
  ASSERT_NO_THROW(dst = std::move(src_2_1));
  ASSERT_NO_THROW(dst = std::move(src_2_2));
  ASSERT_NO_THROW(dst = std::move(src_3_1));
  ASSERT_NO_THROW(dst = std::move(src_3_2));
  ASSERT_THROW(dst = std::move(src_3_3), MaxSizeException);
}

TEST(SLVector, Assign_by_size_and_value) {
  using Container2 = SLVector<int, 2>;

  Container2 dst;

  ASSERT_NO_THROW(dst.assign(0, 0));
  ASSERT_NO_THROW(dst.assign(1, 0));
  ASSERT_NO_THROW(dst.assign(2, 0));
  ASSERT_THROW(dst.assign(3, 0), MaxSizeException);
}

TEST(SLVector, Assign_by_range) {
  using Container2 = SLVector<int, 2>;

  std::vector<int> v0;
  std::vector<int> v1(1);
  std::vector<int> v2(2);
  std::vector<int> v3(3);

  Container2 dst;

  ASSERT_NO_THROW(dst.assign(v0.begin(), v0.end()));
  ASSERT_NO_THROW(dst.assign(v1.begin(), v1.end()));
  ASSERT_NO_THROW(dst.assign(v2.begin(), v2.end()));
  ASSERT_THROW(dst.assign(v3.begin(), v3.end()), MaxSizeException);
}

TEST(SLVector, Assign_by_initialier_list) {
  using Container2 = SLVector<int, 2>;

  Container2 dst;

  ASSERT_NO_THROW(dst.assign({}));
  ASSERT_NO_THROW(dst.assign({1}));
  ASSERT_NO_THROW(dst.assign({1, 2}));
  ASSERT_THROW(dst.assign({1, 2, 3}), MaxSizeException);
}

TEST(SLVector, Emplace_back) {
  using Container2 = SLVector<int, 2>;

  Container2 dst;

  ASSERT_NO_THROW(dst.emplace_back(1));
  EXPECT_EQ(dst.size(), 1);
  ASSERT_NO_THROW(dst.emplace_back(2));
  EXPECT_EQ(dst.size(), 2);
  ASSERT_THROW(dst.emplace_back(3), MaxSizeException);
  EXPECT_EQ(dst.size(), 2);
}

TEST(SLVector, Emplace) {
  using Container3 = SLVector<int, 3>;

  std::vector<int> v_1_2{1, 2};
  std::vector<int> v_0_1_2{0, 1, 2};
  std::vector<int> v_1_0_2{1, 0, 2};
  std::vector<int> v_1_2_0{1, 2, 0};
  std::vector<int> v_1_2_3{1, 2, 3};

  Container3 dst;

  dst = {1, 2};
  EXPECT_EQ(dst, v_1_2);

  ASSERT_NO_THROW(dst.emplace(std::next(dst.begin(), 0), 0));
  EXPECT_EQ(dst, v_0_1_2);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.emplace(std::next(dst.begin(), 1), 0));
  EXPECT_EQ(dst, v_1_0_2);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.emplace(std::next(dst.begin(), 2), 0));
  EXPECT_EQ(dst, v_1_2_0);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.emplace(dst.end(), 0));
  EXPECT_EQ(dst, v_1_2_0);

  dst = {1, 2, 3};
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.emplace(dst.begin(), 0), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.emplace(std::next(dst.begin()), 0), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.emplace(dst.end(), 0), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);
}

TEST(SLVector, Insert_single_value) {
  using Container3 = SLVector<int, 3>;

  std::vector<int> v_1_2{1, 2};
  std::vector<int> v_3_1_2{3, 1, 2};
  std::vector<int> v_1_3_2{1, 3, 2};
  std::vector<int> v_1_2_3{1, 2, 3};

  Container3 dst;

  dst = {1, 2};
  EXPECT_EQ(dst, v_1_2);

  ASSERT_NO_THROW(dst.insert(std::next(dst.begin(), 0), 3));
  EXPECT_EQ(dst, v_3_1_2);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.insert(std::next(dst.begin(), 1), 3));
  EXPECT_EQ(dst, v_1_3_2);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.insert(std::next(dst.begin(), 2), 3));
  EXPECT_EQ(dst, v_1_2_3);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.insert(dst.end(), 3));
  EXPECT_EQ(dst, v_1_2_3);

  dst = {1, 2, 3};
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(dst.begin(), 4), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(std::next(dst.begin()), 4), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(dst.end(), 4), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);
}

TEST(SLVector, Insert_several_value) {
  using Container4 = SLVector<int, 4>;

  std::vector<int> v_1_2{1, 2};
  std::vector<int> v_0_0_1_2{0, 0, 1, 2};
  std::vector<int> v_1_0_0_2{1, 0, 0, 2};
  std::vector<int> v_1_2_0_0{1, 2, 0, 0};
  std::vector<int> v_1_2_3{1, 2, 3};

  Container4 dst;

  dst = {1, 2};
  EXPECT_EQ(dst, v_1_2);

  ASSERT_NO_THROW(dst.insert(std::next(dst.begin(), 0), 2, 0));
  EXPECT_EQ(dst, v_0_0_1_2);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.insert(std::next(dst.begin(), 1), 2, 0));
  EXPECT_EQ(dst, v_1_0_0_2);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.insert(std::next(dst.begin(), 2), 2, 0));
  EXPECT_EQ(dst, v_1_2_0_0);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.insert(dst.end(), 2, 0));
  EXPECT_EQ(dst, v_1_2_0_0);

  dst = {1, 2};
  ASSERT_THROW(dst.insert(dst.begin(), 3, 0), MaxSizeException);
  EXPECT_EQ(dst, v_1_2);

  dst = {1, 2, 3};
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(dst.begin(), 2, 0), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(std::next(dst.begin()), 2, 0), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(dst.end(), 2, 0), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);
}

TEST(SLVector, Insert_by_range) {
  using Container4 = SLVector<int, 4>;

  std::vector<int> v_1_2{1, 2};
  std::vector<int> v_3_4{3, 4};
  std::vector<int> v_3_4_1_2{3, 4, 1, 2};
  std::vector<int> v_1_3_4_2{1, 3, 4, 2};
  std::vector<int> v_1_2_3_4{1, 2, 3, 4};
  std::vector<int> v_1_2_3{1, 2, 3};

  Container4 dst;

  dst = {1, 2};
  EXPECT_EQ(dst, v_1_2);

  ASSERT_NO_THROW(dst.insert(dst.begin(), v_3_4.begin(), v_3_4.end()));
  EXPECT_EQ(dst, v_3_4_1_2);

  dst = {1, 2};
  ASSERT_NO_THROW(
      dst.insert(std::next(dst.begin()), v_3_4.begin(), v_3_4.end()));
  EXPECT_EQ(dst, v_1_3_4_2);

  dst = {1, 2};
  ASSERT_NO_THROW(
      dst.insert(std::next(dst.begin(), 2), v_3_4.begin(), v_3_4.end()));
  EXPECT_EQ(dst, v_1_2_3_4);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.insert(dst.end(), v_3_4.begin(), v_3_4.end()));
  EXPECT_EQ(dst, v_1_2_3_4);

  dst = {1, 2};
  ASSERT_THROW(dst.insert(dst.begin(), v_1_2_3.begin(), v_1_2_3.end()),
               MaxSizeException);
  EXPECT_EQ(dst, v_1_2);

  dst = {1, 2, 3};
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(dst.begin(), v_3_4.begin(), v_3_4.end()),
               MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(std::next(dst.begin()), v_3_4.begin(), v_3_4.end()),
               MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(dst.end(), v_3_4.begin(), v_3_4.end()),
               MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);
}

TEST(SLVector, Insert_by_initializer_list) {
  using Container4 = SLVector<int, 4>;

  std::vector<int> v_1_2{1, 2};
  std::vector<int> v_1_2_3{1, 2, 3};
  std::vector<int> v_3_4{3, 4};
  std::vector<int> v_3_4_1_2{3, 4, 1, 2};
  std::vector<int> v_1_3_4_2{1, 3, 4, 2};
  std::vector<int> v_1_2_3_4{1, 2, 3, 4};

  Container4 dst;

  dst = {1, 2};
  EXPECT_EQ(dst, v_1_2);

  ASSERT_NO_THROW(dst.insert(dst.begin(), {3, 4}));
  EXPECT_EQ(dst, v_3_4_1_2);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.insert(std::next(dst.begin()), {3, 4}));
  EXPECT_EQ(dst, v_1_3_4_2);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.insert(std::next(dst.begin(), 2), {3, 4}));
  EXPECT_EQ(dst, v_1_2_3_4);

  dst = {1, 2};
  ASSERT_NO_THROW(dst.insert(dst.end(), {3, 4}));
  EXPECT_EQ(dst, v_1_2_3_4);

  dst = {1, 2};
  ASSERT_THROW(dst.insert(dst.begin(), {3, 4, 5}), MaxSizeException);
  EXPECT_EQ(dst, v_1_2);

  dst = {1, 2, 3};
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(dst.begin(), {4, 5}), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(std::next(dst.begin()), {4, 5}), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);

  ASSERT_THROW(dst.insert(dst.end(), {4, 5}), MaxSizeException);
  EXPECT_EQ(dst, v_1_2_3);
}

TEST(SLVector, Push_back) {
  using Container2 = SLVector<int, 2>;

  std::vector<int> v1{1};
  std::vector<int> v2{1, 2};

  Container2 dst;
  EXPECT_EQ(dst.size(), 0);

  ASSERT_NO_THROW(dst.push_back(1));
  EXPECT_EQ(dst, v1);

  ASSERT_NO_THROW(dst.push_back(2));
  EXPECT_EQ(dst, v2);

  ASSERT_THROW(dst.push_back(3), MaxSizeException);
  EXPECT_EQ(dst, v2);
}

TEST(SLVector, Reserve) {
  using Container2 = SLVector<int, 2>;

  Container2 dst;
  EXPECT_EQ(dst.size(), 0);
  EXPECT_EQ(dst.capacity(), 0);

  ASSERT_NO_THROW(dst.reserve(1));
  EXPECT_EQ(dst.capacity(), 1);

  ASSERT_NO_THROW(dst.reserve(2));
  EXPECT_EQ(dst.capacity(), 2);

  ASSERT_THROW(dst.reserve(3), MaxSizeException);
  EXPECT_EQ(dst.capacity(), 2);
}

TEST(SLVector, Resize) {
  using Container2 = SLVector<int, 2>;

  Container2 dst;
  EXPECT_EQ(dst.size(), 0);

  ASSERT_NO_THROW(dst.resize(1));
  EXPECT_EQ(dst.size(), 1);

  ASSERT_NO_THROW(dst.resize(2));
  EXPECT_EQ(dst.size(), 2);

  ASSERT_THROW(dst.resize(3), MaxSizeException);
  EXPECT_EQ(dst.size(), 2);
}

TEST(SLVector, Resize_with_value) {
  using Container2 = SLVector<int, 2>;

  std::vector<int> v1{100};
  std::vector<int> v2{100, 200};

  Container2 dst;

  EXPECT_EQ(dst.size(), 0);

  ASSERT_NO_THROW(dst.resize(1, 100));
  EXPECT_EQ(dst, v1);

  ASSERT_NO_THROW(dst.resize(2, 200));
  EXPECT_EQ(dst, v2);

  ASSERT_THROW(dst.resize(3, 300), MaxSizeException);
  EXPECT_EQ(dst, v2);
}
