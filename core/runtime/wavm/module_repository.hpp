/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_HPP
#define KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_HPP

#include <memory>

#include <gsl/span>

#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"

namespace kagome::runtime {
  class RuntimeCodeProvider;
}

namespace kagome::runtime::wavm {

  class ModuleInstance;
  class Module;
  class Memory;

  class ModuleRepository {
   public:
    virtual ~ModuleRepository() = default;

    virtual outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        std::shared_ptr<const RuntimeCodeProvider> code_provider,
        const primitives::BlockInfo &block) = 0;

    virtual outcome::result<std::unique_ptr<Module>> loadFrom(
        gsl::span<const uint8_t> byte_code) = 0;

  };

}  // namespace kagome::runtime::wavm

#endif  // KAGOME_CORE_RUNTIME_WAVM_IMPL_MODULE_REPOSITORY_HPP
