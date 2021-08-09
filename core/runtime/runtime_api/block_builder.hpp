/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_BLOCK_BUILDER_HPP
#define KAGOME_RUNTIME_BLOCK_BUILDER_HPP

#include <list>

#include <outcome/outcome.hpp>
#include "primitives/apply_result.hpp"
#include "primitives/block.hpp"
#include "primitives/block_header.hpp"
#include "primitives/check_inherents_result.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/inherent_data.hpp"
#include "runtime/persistent_result.hpp"

namespace kagome::runtime {
  /**
   * Part of runtime API responsible for building a block for a runtime.
   */
  class BlockBuilder {
   public:
    virtual ~BlockBuilder() = default;

    /**
     * Apply the given extrinsic.
     */
    virtual outcome::result<PersistentResult<primitives::ApplyExtrinsicResult>>
    apply_extrinsic(const primitives::BlockInfo &block,
                    storage::trie::RootHash const &storage_hash,
                    const primitives::Extrinsic &extrinsic) = 0;

    /**
     * Finish the current block.
     */
    virtual outcome::result<primitives::BlockHeader> finalize_block(
        const primitives::BlockInfo &block,
        storage::trie::RootHash const &storage_hash) = 0;

    /**
     * Generate inherent extrinsics. The inherent data will vary from chain to
     * chain.
     */
    virtual outcome::result<std::vector<primitives::Extrinsic>>
    inherent_extrinsics(const primitives::BlockInfo &block,
                        storage::trie::RootHash const &storage_hash,
                        const primitives::InherentData &data) = 0;

    /**
     * Check that the inherents are valid. The inherent data will vary from
     * chain to chain.
     */
    virtual outcome::result<primitives::CheckInherentsResult> check_inherents(
        const primitives::Block &block,
        const primitives::InherentData &data) = 0;

    /**
     * Generate a random seed.
     */
    virtual outcome::result<common::Hash256> random_seed(
        const primitives::BlockHash &block) = 0;
  };
}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_BLOCK_BUILDER_HPP
