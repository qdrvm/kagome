/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <span>

#include "outcome/outcome.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/types.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {

  class Module;

  struct CompilationError : std::runtime_error {
    CompilationError(const std::string &message)
        : std::runtime_error(message.c_str()) {}

    std::string_view message() const {
      return what();
    }
  };

  inline std::error_code make_error_code(CompilationError) {
    return Error::COMPILATION_FAILED;
  }

  inline void outcome_throw_as_system_error_with_payload(CompilationError e) {
    throw e;
  }

  class ModuleFactory {
   public:
    virtual ~ModuleFactory() = default;

    virtual outcome::result<std::shared_ptr<Module>, CompilationError> make(
        common::BufferView code) const = 0;
  };

}  // namespace kagome::runtime
