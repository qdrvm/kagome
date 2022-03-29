/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_MODULE_HPP
#define KAGOME_CORE_RUNTIME_MODULE_HPP

#include <optional>

#include "outcome/outcome.hpp"
#include "runtime/instance_environment.hpp"

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
  };

  struct SingleModuleCache {
    SingleModuleCache() : module{std::nullopt} {};
    std::optional<std::shared_ptr<Module>> module;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_MODULE_HPP
