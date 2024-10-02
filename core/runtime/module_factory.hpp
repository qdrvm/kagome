/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>

#include "common/buffer_view.hpp"
#include "outcome/custom.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {

  class Module;

  struct CompilationError : std::runtime_error {
    CompilationError(const std::string &message)
        : std::runtime_error(message.c_str()) {}

    CompilationError(const std::error_code &ec)
        : CompilationError{ec.message()} {}

    std::string_view message() const {
      return what();
    }
  };

  inline std::error_code make_error_code(CompilationError) {
    return Error::COMPILATION_FAILED;
  }
  template <typename R>
  using CompilationOutcome = CustomOutcome<R, CompilationError>;

  inline void outcome_throw_as_system_error_with_payload(CompilationError e) {
    throw e;
  }

  class ModuleFactory {
   public:
    virtual ~ModuleFactory() = default;

    /**
     * Used as part of filename to separate files of different incompatible
     * compilers.
     * `std::nullopt` means `path_compiled` stores raw WASM code for
     * interpretation.
     */
    virtual std::optional<std::string_view> compilerType() const = 0;

    /**
     * Compile `wasm` code to `path_compiled`.
     */
    virtual CompilationOutcome<void> compile(
        std::filesystem::path path_compiled,
        BufferView wasm,
        const RuntimeContext::ContextParams &config) const = 0;

    /**
     * Load compiled code from `path_compiled`.
     */
    virtual CompilationOutcome<std::shared_ptr<Module>> loadCompiled(
        std::filesystem::path path_compiled) const = 0;
  };

}  // namespace kagome::runtime
