/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_BASIC_AUTHORSHIP_BLOCK_BUILDER_FACTORY_HPP
#define KAGOME_CORE_BASIC_AUTHORSHIP_BLOCK_BUILDER_FACTORY_HPP

#include "basic_authorship/block_builder.hpp"
#include "primitives/block_id.hpp"
#include "runtime/block_builder_api.hpp"
#include "runtime/core.hpp"

namespace kagome::basic_authorship {

  /**
   * Creates new block builders
   */
  class BlockBuilderFactory {
   public:
    virtual ~BlockBuilderFactory() = default;

    /**
     * Prepares BlockBuilder for creating block on top of parent block and using
     * provided digest
     */
    virtual outcome::result<std::unique_ptr<BlockBuilder>> create(
        const primitives::BlockId &parent_id,
        const primitives::Digest &inherent_digest) = 0;
  };

}  // namespace kagome::basic_authorship

#endif  // KAGOME_CORE_BASIC_AUTHORSHIP_BLOCK_BUILDER_FACTORY_HPP
