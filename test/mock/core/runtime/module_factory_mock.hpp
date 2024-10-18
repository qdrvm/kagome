/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module_factory.hpp"

#include <gmock/gmock.h>

#include "runtime/module.hpp"

namespace kagome::runtime {

  class ModuleFactoryMock final : public ModuleFactory {
   public:
    MOCK_METHOD(std::optional<std::string_view>,
                compilerType,
                (),
                (const, override));
    MOCK_METHOD((CompilationOutcome<void>),
                compile,
                (std::filesystem::path,
                 BufferView,
                 const RuntimeContext::ContextParams &),
                (const, override));
    MOCK_METHOD((CompilationOutcome<std::shared_ptr<Module>>),
                loadCompiled,
                (std::filesystem::path, const RuntimeContext::ContextParams &),
                (const, override));
  };
}  // namespace kagome::runtime
