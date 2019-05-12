/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_BUILDER_HPP
#define KAGOME_BLOCK_BUILDER_HPP

#include <list>

#include <outcome/outcome.hpp>
#include "primitives/block.hpp"
#include "primitives/block_header.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/inherent_data.hpp"

namespace kagome::runtime {

  struct CheckInherentsResult {
    /// Did the check succeed?
    bool okay_ = false;
    /// Did we encounter a fatal error?
    bool fatal_error_ = false;
    /// We use the `InherentData` to store our errors.
    primitives::InherentData errors_;
  };

  /**
   * Part of runtime API responsible for building a block for a runtime.
   */
  class BlockBuilder {
   public:
    virtual ~BlockBuilder() = default;

    /// Apply the given extrinsics.
    virtual outcome::result<bool> apply_extrinsic(
        primitives::Extrinsic extrinsic) = 0;

    /// Finish the current block.
    virtual outcome::result<primitives::BlockHeader> finalize_block() = 0;

    /// Generate inherent extrinsics. The inherent data will vary from chain to chain.
    virtual outcome::result<std::vector<primitives::Extrinsic>>
    inherent_extrinsics(primitives::InherentData data) = 0;

    /// Check that the inherents are valid. The inherent data will vary from chain to chain.
    virtual outcome::result<CheckInherentsResult> check_inherents(
        primitives::Block block, primitives::InherentData data) = 0;

    /// Generate a random seed.
    virtual outcome::result<common::Hash256> random_seed() = 0;
  };

} // namespace kagome::runtime

#endif  // KAGOME_BLOCK_BUILDER_HPP
