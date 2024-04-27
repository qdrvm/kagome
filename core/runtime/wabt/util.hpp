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

#include "common/buffer.hpp"
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

  inline WabtOutcome<wabt::Module> wabtDecode(common::BufferView code) {
    wabt::Module module;
    OUTCOME_TRY(wabtTry([&](wabt::Errors &errors) {
      return wabt::ReadBinaryIr(
          "",
          code.data(),
          code.size(),
          wabt::ReadBinaryOptions({}, nullptr, true, false, false),
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
}  // namespace kagome::runtime
