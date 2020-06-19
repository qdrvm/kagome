/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_CORE_RUNTIME_WASM_RESULT_TEST_CPP
#define KAGOME_TEST_CORE_RUNTIME_WASM_RESULT_TEST_CPP

#include "runtime/wasm_result.hpp"

#include <cstdint>

#include <gtest/gtest.h>

using kagome::runtime::WasmResult;

struct TestCase {
  int64_t res;
  std::pair<uint32_t, uint32_t> wres;
};

class WasmResultTest : public ::testing::TestWithParam<TestCase> {
 public:
};

/**
 * @given test params: result of wasm execution (int64_t) and result of
 * decomposition: pair of uint32_t
 * @when WasmResult is constructed on provided wasm execution result
 * @then its address, value pair matches provided pair of uint32_t values
 */
TEST_P(WasmResultTest, DecomposeSuccess) {
  auto [res, pair] = GetParam();
  auto [address, length] = pair;

  WasmResult r(res);
  ASSERT_EQ(r.address, address);
  ASSERT_EQ(r.length, length);
}

INSTANTIATE_TEST_CASE_P(WasmResultTestCases, WasmResultTest,
                        ::testing::Values(TestCase{0, {0, 0}},
                                          TestCase{1, {1, 0}},
                                          TestCase{4294967297, {1, 1}},
                                          TestCase{4294967296, {0, 1}},
                                          TestCase{9223372036854775807ll,
                                                   {4294967295, 2147483647}}));

#endif  // KAGOME_TEST_CORE_RUNTIME_WASM_RESULT_TEST_CPP
