/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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

  /**
   * A wrapper for compiled module. Used when we update WAVM runtime in order to
   * skip double compilation which takes significant time (see issue #1104).
   * Currently it's shared through DI.
   */
  class SingleModuleCache {
   public:
    /**
     * @brief Sets new cached value, scrapping previous if any
     * @param module New compiled module to store
     */
    void set(std::shared_ptr<Module> module) {
      module_ = module;
    }

    /**
     * Pops stored module (if any), clears cache in process.
     * @return Value if any, std::nullopt otherwise.
     */
    std::optional<std::shared_ptr<Module>> try_extract() {
      auto module = module_;
      module_.reset();
      return module;
    }

   private:
    std::optional<std::shared_ptr<Module>> module_;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_MODULE_HPP
