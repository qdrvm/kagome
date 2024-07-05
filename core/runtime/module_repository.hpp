/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <span>

#include "host_api/host_api.hpp"
#include "outcome/outcome.hpp"
#include "primitives/block_data.hpp"
#include "primitives/version.hpp"

namespace kagome::runtime {

  class ModuleInstance;
  class Module;
  class Memory;
  class RuntimeCodeProvider;

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
     * @param block info of the block at which the runtime code should be
     * extracted
     * @param state_hash of the block at which the runtime code should be
     * extracted
     */
    virtual outcome::result<std::shared_ptr<ModuleInstance>> getInstanceAt(
        const primitives::BlockInfo &block,
        const storage::trie::RootHash &state_hash) = 0;

    /**
     * Return cached `readEmbeddedVersion` result.
     */
    virtual outcome::result<std::optional<primitives::Version>> embeddedVersion(
        const primitives::BlockHash &block_hash) = 0;
  };

}  // namespace kagome::runtime
