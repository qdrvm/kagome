/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_CORE_HPP
#define KAGOME_RUNTIME_CORE_HPP

#include <vector>

#include <optional>
#include <outcome/outcome.hpp>

#include "primitives/block.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"
#include "primitives/transaction_validity.hpp"
#include "primitives/version.hpp"
#include "runtime/runtime_context.hpp"
#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::runtime {
  class ModuleInstance;
  class RuntimeCodeProvider;
  class RuntimeContext;

  // interface for calls that are done by the runtime via Host API
  // (typically Runtime API is the contrary -- calls from host to runtime)
  class RestrictedCore {
   public:
    virtual ~RestrictedCore() = default;

    /**
     * @brief Returns the version of the runtime - version for nested calls,
     * such as in MiscExtension
     * @return runtime version
     */
    virtual outcome::result<primitives::Version> version() = 0;
  };

  class Core {
   public:
    virtual ~Core() = default;

    /**
     * @brief Returns the version of the runtime
     * @return runtime version
     */
    virtual outcome::result<primitives::Version> version(
        std::shared_ptr<ModuleInstance> instance) = 0;

    /**
     * @brief Returns the version of the runtime
     * @return runtime version
     */
    virtual outcome::result<primitives::Version> version(
        const primitives::BlockHash &block) = 0;

    /**
     * @brief Executes the given block
     * @param block block to execute
     * @param changes_tracker storage writes and deletes tracker
     */
    virtual outcome::result<void> execute_block(
        const primitives::Block &block,
        TrieChangesTrackerOpt changes_tracker) = 0;
    virtual outcome::result<void> execute_block_ref(
        const primitives::BlockReflection &block,
        TrieChangesTrackerOpt changes_tracker) = 0;

    /**
     * @brief Initialize a block with the given header.
     * @param header header used for block initialization
     * @param changes_tracker storage writes and deletes tracker
     */
    virtual outcome::result<std::unique_ptr<RuntimeContext>> initialize_block(
        const primitives::BlockHeader &header,
        TrieChangesTrackerOpt changes_tracker) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_CORE_HPP
