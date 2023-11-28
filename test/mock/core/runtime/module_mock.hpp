/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/module.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class ModuleMock : public Module {
   public:
    MOCK_METHOD(std::shared_ptr<ModuleInstance>,
                instantiate,
                (),
                (const));
  };
}  // namespace kagome::runtime
