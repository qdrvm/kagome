/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_MODULE_HPP
#define KAGOME_CORE_RUNTIME_MODULE_HPP

#include "outcome/outcome.hpp"

namespace kagome::runtime {

  class ModuleInstance;

  class Module {
   public:
    virtual ~Module() = default;

    virtual outcome::result<std::unique_ptr<ModuleInstance>> instantiate()
        const = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_MODULE_HPP
