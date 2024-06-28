/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "outcome/outcome.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {

  class ModuleInstance;

  /**
   * A WebAssembly code module.
   * Contains a set of exported objects (e. g. functions and variable
   * declarations) and imported objects (e. g. Host API functions in case of
   * Polkadot).
   */
  class Module {
   public:
    virtual ~Module() = default;

    virtual outcome::result<std::shared_ptr<ModuleInstance>> instantiate()
        const = 0;

    virtual WasmSize getInitialMemorySize() const = 0;
    virtual std::optional<WasmSize> getMaxMemorySize() const = 0;
  };
}  // namespace kagome::runtime
