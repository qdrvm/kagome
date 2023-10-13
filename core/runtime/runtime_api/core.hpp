/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "primitives/block.hpp"
#include "primitives/common.hpp"
#include "primitives/version.hpp"
#include "runtime/runtime_context.hpp"
#include "storage/changes_trie/changes_tracker.hpp"

namespace kagome::runtime {
  /**
   * Core represents mandatory part of runtime api
   */
  class Core {
   public:
    virtual ~Core() = default;

    /**
     * @brief Returns the version of the runtime
     * @return runtime version
     */
    virtual outcome::result<primitives::Version> version(
        const primitives::BlockHash &block) = 0;

    /**
     * @brief Returns the version of the runtime - version for nested calls,
     * such as in MiscExtension
     * @return runtime version
     */
    virtual outcome::result<primitives::Version> version() = 0;

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
