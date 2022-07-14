/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_MODULE_FACTORY_HPP
#define KAGOME_CORE_RUNTIME_MODULE_FACTORY_HPP

#include <gsl/span>

#include "outcome/outcome.hpp"
#include "runtime/instance_environment.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {

  class Module;

  class ModuleFactory {
   public:
    virtual ~ModuleFactory() = default;

    virtual outcome::result<std::unique_ptr<Module>> make(
        gsl::span<const uint8_t> code) const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_MODULE_FACTORY_HPP
