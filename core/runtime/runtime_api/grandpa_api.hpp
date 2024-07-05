/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/types/authority.hpp"
#include "consensus/grandpa/types/equivocation_proof.hpp"
#include "primitives/common.hpp"

namespace kagome::runtime {
  // https://github.com/paritytech/substrate/blob/8bf08ca63491961fafe6adf414a7411cb3953dcf/core/finality-grandpa/primitives/src/lib.rs#L56

  /**
   * @brief interface for Grandpa runtime functions
   */
  class GrandpaApi {
   protected:
    using Authorities = consensus::grandpa::Authorities;
    using AuthoritySetId = consensus::grandpa::AuthoritySetId;

   public:
    virtual ~GrandpaApi() = default;

    /**
     * @brief calls Grandpa_authorities runtime api function
     * @return collection of current grandpa authorities with their weights
     */
    virtual outcome::result<Authorities> authorities(
        const primitives::BlockHash &block_hash) = 0;

    /**
     * @return the id of the current voter set at the provided block
     */
    virtual outcome::result<AuthoritySetId> current_set_id(
        const primitives::BlockHash &block) = 0;

    virtual outcome::result<
        std::optional<consensus::grandpa::OpaqueKeyOwnershipProof>>
    generate_key_ownership_proof(
        const primitives::BlockHash &block_hash,
        consensus::SlotNumber slot,
        consensus::grandpa::AuthorityId authority_id) = 0;

    virtual outcome::result<void> submit_report_equivocation_unsigned_extrinsic(
        const primitives::BlockHash &block_hash,
        consensus::grandpa::EquivocationProof equivocation_proof,
        consensus::grandpa::OpaqueKeyOwnershipProof key_owner_proof) = 0;
  };

}  // namespace kagome::runtime
