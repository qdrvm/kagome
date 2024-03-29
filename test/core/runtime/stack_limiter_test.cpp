/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <wabt/binary-reader-ir.h>
#include <wabt/binary-reader.h>
#include <wabt/binary-writer.h>
#include <wabt/ir.h>
#include <wabt/stream.h>
#include <wabt/wast-lexer.h>
#include <wabt/wast-parser.h>
#include <wabt/wat-writer.h>

#include "common/bytestr.hpp"
#include "log/logger.hpp"
#include "runtime/wabt/instrument.hpp"
#include "runtime/wabt/stack_limiter.hpp"
#include "runtime/wabt/util.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

static constexpr uint32_t ACTIVATION_FRAME_COST = 2;

using kagome::byte2str;
using kagome::HeapAllocStrategy;
using kagome::HeapAllocStrategyDynamic;
using kagome::HeapAllocStrategyStatic;
using kagome::str2byte;
using kagome::runtime::convertMemoryImportIntoExport;
using kagome::runtime::setupMemoryAccordingToHeapAllocStrategy;
using kagome::runtime::wabtDecode;
using kagome::runtime::wabtEncode;

std::unique_ptr<wabt::Module> wat_to_module(std::span<const uint8_t> wat) {
  wabt::Result result;
  wabt::Errors errors;
  std::unique_ptr<wabt::WastLexer> lexer =
      wabt::WastLexer::CreateBufferLexer("", wat.data(), wat.size(), &errors);
  if (Failed(result)) {
    throw std::runtime_error{"Failed to parse WAT"};
  }

  std::unique_ptr<wabt::Module> module;
  wabt::WastParseOptions parse_wast_options{{}};
  result =
      wabt::ParseWatModule(lexer.get(), &module, &errors, &parse_wast_options);
  if (Failed(result)) {
    throw std::runtime_error{"Failed to parse module"};
  }
  return module;
}

std::vector<uint8_t> wat_to_wasm(std::span<const uint8_t> wat) {
  auto module = wat_to_module(wat);
  wabt::MemoryStream stream;
  if (wabt::Failed(wabt::WriteBinaryModule(
          &stream,
          module.get(),
          wabt::WriteBinaryOptions{{}, true, false, true}))) {
    throw std::runtime_error{"Failed to write binary wasm"};
  }
  return std::move(stream.output_buffer().data);
}

auto fromWat(std::string_view wat) {
  return wat_to_module(str2byte(wat));
}

std::string toWat(const wabt::Module &module) {
  wabt::MemoryStream s;
  EXPECT_TRUE(
      wabt::Succeeded(wabt::WriteWat(&s, &module, wabt::WriteWatOptions{})));
  return std::string{byte2str(s.output_buffer().data)};
}

void expectWasm(const wabt::Module &actual, std::string_view expected) {
  auto expected_fmt = toWat(*fromWat(expected));
  EXPECT_EQ(toWat(actual), expected_fmt);
  auto actual2 = wabtDecode(wabtEncode(actual).value()).value();
  EXPECT_EQ(toWat(actual2), expected_fmt);
}

uint32_t compute_cost(std::string_view data) {
  auto module = fromWat(data);
  EXPECT_OUTCOME_TRUE(cost,
                      kagome::runtime::detail::compute_stack_cost(
                          kagome::log::createLogger("StackLimiterTest"),
                          *module->funcs[0],
                          *module));
  return cost;
}

TEST(StackLimiterTest, simple_test) {
  std::string_view data = R"(
    (module
      (func
        i32.const 1
        i32.const 2
        i32.const 3
        drop
        drop
        drop
      )
    ))";
  testutil::prepareLoggers();
  ASSERT_EQ(compute_cost(data), 3 + ACTIVATION_FRAME_COST);
}

TEST(StackLimiterTest, implicit_and_explicit_return) {
  std::string_view data = R"((module(func(result i32) i32.const 0 return)))";
  testutil::prepareLoggers();
  ASSERT_EQ(compute_cost(data), 1 + ACTIVATION_FRAME_COST);
}

TEST(StackLimiterTest, dont_count_in_unreachable) {
  std::string_view data =
      R"((module(memory 0)(func(result i32) unreachable memory.grow)))";
  testutil::prepareLoggers();
  ASSERT_EQ(compute_cost(data), ACTIVATION_FRAME_COST);
}

TEST(StackLimiterTest, yet_another_test) {
  std::string_view data = R"(
  (module(memory 0)(
    func;; Push two values and then pop them.
        ;; This will make max depth to be equal to 2.
    i32.const 0
    i32.const 1
    drop
    drop
    ;; Code after `unreachable` shouldn't have an effect
    ;; on the max depth.
    unreachable
    i32.const 0
    i32.const 1
    i32.const 2
  )))";
  testutil::prepareLoggers();
  ASSERT_EQ(compute_cost(data), 2 + ACTIVATION_FRAME_COST);
}

TEST(StackLimiterTest, call_indirect) {
  std::string_view data =
      R"(
    (module
      (table $ptr 1 1 funcref)
      (elem $ptr(i32.const 0) func 1)
      (func $main
        (call_indirect(i32.const 0))
        (call_indirect(i32.const 0))
        (call_indirect(i32.const 0))
      )
      (func $callee i64.const 42 drop)
    ))";
  testutil::prepareLoggers();
  ASSERT_EQ(compute_cost(data), 1 + ACTIVATION_FRAME_COST);
}

TEST(StackLimiterTest, breaks) {
  std::string_view data =
      R"(
  (module
    (func $main block(result i32)
      block(result i32)
      i32.const 99
      br 1
      end
      end
      drop
    )
  ))";
  testutil::prepareLoggers();
  ASSERT_EQ(compute_cost(data), 1 + ACTIVATION_FRAME_COST);
}

TEST(StackLimiterTest, if_else_works) {
  std::string_view data =
      R"(
  (module
    (func $main
      i32.const 7
      i32.const 1
      if (result i32)
        i32.const 42
      else
        i32.const 99
      end
      i32.const 97
      drop
      drop
      drop
    )
  ))";
  testutil::prepareLoggers();
  ASSERT_EQ(compute_cost(data), 3 + ACTIVATION_FRAME_COST);
}

class StackLimiterCompareTest : public testing::TestWithParam<std::string> {
 public:
  void SetUp() override {
    testutil::prepareLoggers();
  }
  // You can implement all the usual fixture class members here.
  // To access the test parameter, call GetParam() from class
  // TestWithParam<T>.
};

TEST_P(StackLimiterCompareTest, output_matches_expected) {
  using namespace std::string_literals;
  using namespace std::string_view_literals;
  auto base =
      kagome::filesystem::path(__FILE__).parent_path() / "stack_limiter";
  std::filesystem::path expected =
      base / "expectations" / (GetParam() + ".wat");
  std::filesystem::path fixture = base / "fixtures" / (GetParam() + ".wat");

  std::vector<uint8_t> expected_wat;
  std::vector<uint8_t> fixture_wat;
  ASSERT_TRUE(wabt::Succeeded(wabt::ReadFile(expected.c_str(), &expected_wat)));
  ASSERT_TRUE(wabt::Succeeded(wabt::ReadFile(fixture.c_str(), &fixture_wat)));

  auto fixture_wasm = wat_to_wasm(fixture_wat);
  auto expected_module = wat_to_module(expected_wat);

  EXPECT_OUTCOME_TRUE(
      result_wasm,
      kagome::runtime::instrumentWithStackLimiter(fixture_wasm, 1024));

  wabt::Module result_module;
  wabt::Errors errors;
  auto result = wabt::ReadBinaryIr(
      "",
      result_wasm.data(),
      result_wasm.size(),
      wabt::ReadBinaryOptions{{}, nullptr, true, false, false},
      &errors,
      &result_module);

  if (Failed(result)) {
    throw std::runtime_error{"Failed to read binary module"};
  }

  if (toWat(result_module) != toWat(*expected_module)) {
    std::filesystem::create_directories(std::filesystem::temp_directory_path()
                                        / "kagome_test");
    auto base_path = std::filesystem::temp_directory_path() / "kagome_test";

    auto result_file = base_path / (GetParam() + ".result.wat");
    wabt::FileStream result_file_stream{result_file.c_str()};
    ASSERT_TRUE(wabt::Succeeded(wabt::WriteWat(
        &result_file_stream, &result_module, wabt::WriteWatOptions{})));

    auto expected_file = base_path / (GetParam() + ".expected.wat");
    wabt::FileStream expected_file_stream{expected_file.c_str()};
    ASSERT_TRUE(wabt::Succeeded(wabt::WriteWat(&expected_file_stream,
                                               expected_module.get(),
                                               wabt::WriteWatOptions{})));
    FAIL() << "Result doesn't match expected: diff -y " << result_file << " "
           << expected_file;
  }
}

INSTANTIATE_TEST_SUITE_P(SuiteFromSubstrate,
                         StackLimiterCompareTest,
                         testing::Values("empty_functions",
                                         "global",
                                         "imports",
                                         "many_locals",
                                         "simple",
                                         "start",
                                         "table"));

auto wat_memory_import = R"(
  (module
    (import "env" "mem" (memory (;0;) 100)))
)";
auto wat_memory_export = R"(
  (module
    (memory (;0;) 100)
    (export "mem" (memory 0)))
)";
auto memory_limit_static = std::make_pair(HeapAllocStrategyStatic{100}, R"(
  (module
    (memory (;0;) 200 200)
    (export "mem" (memory 0)))
)");

TEST(WasmInstrumentTest, memory_import) {
  auto module = fromWat(wat_memory_import);
  convertMemoryImportIntoExport(*module).value();
  expectWasm(*module, wat_memory_export);
}

TEST(WasmInstrumentTest, memory_limit) {
  auto test = [](HeapAllocStrategy config, std::string_view expected) {
    auto module = fromWat(wat_memory_export);
    setupMemoryAccordingToHeapAllocStrategy(*module, config).value();
    expectWasm(*module, expected);
  };
  test(HeapAllocStrategyDynamic{}, wat_memory_export);
  test(HeapAllocStrategyDynamic{200}, R"(
    (module
      (memory (;0;) 100 200)
      (export "mem" (memory 0)))
  )");
  test(memory_limit_static.first, memory_limit_static.second);
}

TEST(WasmInstrumentTest, memory_import_limit) {
  auto module = fromWat(wat_memory_import);
  convertMemoryImportIntoExport(*module).value();
  setupMemoryAccordingToHeapAllocStrategy(*module, memory_limit_static.first)
      .value();
  expectWasm(*module, memory_limit_static.second);
}
