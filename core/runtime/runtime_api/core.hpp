/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_CORE_HPP
#define KAGOME_RUNTIME_CORE_HPP

#include <vector>

#include <boost/optional.hpp>
#include <outcome/outcome.hpp>

#include "primitives/authority.hpp"
#include "primitives/block.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"
#include "primitives/transaction_validity.hpp"
#include "primitives/version.hpp"

namespace kagome::runtime {
  class RuntimeCodeProvider;

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
    virtual outcome::result<primitives::Version> versionAt(
        primitives::BlockHash const &block) = 0;
    virtual outcome::result<primitives::Version> version() = 0;

    /**
     * @brief Executes the given block
     * @param block block to execute
     */
    virtual outcome::result<void> execute_block(
        const primitives::Block &block) = 0;

    /**
     * @brief Initialize a block with the given header.
     * @param header header used for block initialization
     */
    virtual outcome::result<void> initialise_block(
        const primitives::BlockHeader &header) = 0;

    /**
     * Get current authorities
     * @return collection of authorities
     */
    virtual outcome::result<std::vector<primitives::AuthorityId>> authorities(
        const primitives::BlockId &block_id) = 0;
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_CORE_HPP
