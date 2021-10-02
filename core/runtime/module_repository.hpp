/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_MODULE_REPOSITORY_HPP
#define KAGOME_CORE_RUNTIME_IMPL_MODULE_REPOSITORY_HPP

#include <memory>

#include <gsl/span>

#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"

namespace kagome::runtime {
  class RuntimeCodeProvider;
}

namespace kagome::runtime {

  class ModuleInstance;
  class Module;
  class Memory;

  /**
   * Repository for runtime modules
   * Allows loading and compiling a module directly from its web assembly byte
   * code and instantiating a runtime module at an arbitrary block
   */
  class ModuleRepository {
   public:
    virtual ~ModuleRepository() = default;

    /**
     * @brief Returns a module instance for runtime at the \arg block state,
     * loading its code using the provided \arg code_provider
     * @param code_provider the code provider used to extract the runtime code
     * from the given block
     * @param block info of the block at which the runtime code should be
     * extracted
     */
    virtual outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        std::shared_ptr<const RuntimeCodeProvider> code_provider,
        const primitives::BlockInfo &block,
        const primitives::BlockHeader &header) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_MODULE_REPOSITORY_HPP
