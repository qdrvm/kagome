/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

  WasmResult r{res};
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
