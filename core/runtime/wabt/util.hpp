/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <wabt/binary-reader-ir.h>
#include <wabt/binary-writer.h>
#include <wabt/error-formatter.h>
#include <wabt/validator.h>
#include <wabt/wast-lexer.h>
#include <wabt/wast-parser.h>
#include "wabt/option-parser.h"

#include "common/buffer.hpp"
#include "common/bytestr.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/wabt/error.hpp"

namespace kagome::runtime {
  WabtOutcome<void> wabtTry(auto &&f) {
    wabt::Errors errors;
    if (wabt::Failed(f(errors))) {
      return WabtError{
          wabt::FormatErrorsToString(errors, wabt::Location::Type::Binary)};
    }
    return outcome::success();
  }

  struct WasmFeatures {
    bool ext_bulk_memory{false};
  };

  inline WabtOutcome<wabt::Module> wabtDecode(
      common::BufferView code, const WasmFeatures &requested_features) {
    wabt::Module module;
    wabt::Features features;
    features.disable_reference_types();
    if (not requested_features.ext_bulk_memory) {
      features.disable_bulk_memory();
    }
    OUTCOME_TRY(wabtTry([&](wabt::Errors &errors) {
      return wabt::ReadBinaryIr(
          "",
          code.data(),
          code.size(),
          wabt::ReadBinaryOptions(features, nullptr, true, false, false),
          &errors,
          &module);
    }));
    return module;
  }

  inline WabtOutcome<void> wabtValidate(const wabt::Module &module) {
    return wabtTry([&](wabt::Errors &errors) {
      return wabt::ValidateModule(&module, &errors, wabt::ValidateOptions{});
    });
  }

  inline WabtOutcome<common::Buffer> wabtEncode(const wabt::Module &module) {
    wabt::MemoryStream s;
    if (wabt::Failed(wabt::WriteBinaryModule(
            &s, &module, wabt::WriteBinaryOptions({}, false, false, true)))) {
      return WabtError{"Failed to serialize WASM module"};
    }
    return common::Buffer{std::move(s.output_buffer().data)};
  }

  inline std::unique_ptr<wabt::Module> watToModule(
      std::span<const uint8_t> wat) {
    wabt::Result result;
    wabt::Errors errors;
    std::unique_ptr<wabt::WastLexer> lexer =
        wabt::WastLexer::CreateBufferLexer("", wat.data(), wat.size(), &errors);
    if (Failed(result)) {
      throw std::runtime_error{"Failed to parse WAT"};
    }

    std::unique_ptr<wabt::Module> module;
    wabt::WastParseOptions parse_wast_options{{}};
    result = wabt::ParseWatModule(
        lexer.get(), &module, &errors, &parse_wast_options);
    if (Failed(result)) {
      throw std::runtime_error{"Failed to parse module"};
    }
    if (module == nullptr) {
      throw std::runtime_error{"Module is null"};
    }
    return module;
  }

  inline std::vector<uint8_t> watToWasm(std::span<const uint8_t> wat) {
    auto module = watToModule(wat);
    return wabtEncode(*module).value();
  }

  inline auto fromWat(std::string_view wat) {
    return watToModule(str2byte(wat));
  }

}  // namespace kagome::runtime
