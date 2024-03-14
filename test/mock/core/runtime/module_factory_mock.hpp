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
    MOCK_METHOD((outcome::result<std::shared_ptr<Module>, CompilationError>),
                make,
                (common::BufferView),
                (const, override));

    MOCK_METHOD(bool, testDontInstrument, (), (const, override));
  };
}  // namespace kagome::runtime
