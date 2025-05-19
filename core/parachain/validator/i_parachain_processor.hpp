/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/types/collator_messages_vstaging.hpp"
#include "outcome/outcome.hpp"
#include "parachain/validator/statement_distribution/types.hpp"

namespace kagome::parachain {

  class ParachainStorage {
   public:
    virtual ~ParachainStorage() = default;

    virtual outcome::result<network::FetchChunkResponse> OnFetchChunkRequest(
        const network::FetchChunkRequest &request) = 0;

    virtual outcome::result<network::FetchChunkResponseObsolete>
    OnFetchChunkRequestObsolete(const network::FetchChunkRequest &request) = 0;

    /**
     * @brief Fetches the Proof of Validity (PoV) for a given candidate.
     *
     * @param candidate_hash The hash of the candidate for which the PoV is to
     * be fetched.
     * @return network::ResponsePov The PoV associated with the given candidate
     * hash.
     */
    virtual network::ResponsePov getPov(CandidateHash &&candidate_hash) = 0;
  };

  class ParachainProcessor {
   public:
    enum class Error : uint8_t {
      RESPONSE_ALREADY_RECEIVED = 1,
      COLLATION_NOT_FOUND,
      KEY_NOT_PRESENT,
      VALIDATION_FAILED,
      VALIDATION_SKIPPED,
      OUT_OF_VIEW,
      DUPLICATE,
      NO_INSTANCE,
      NOT_A_VALIDATOR,
      NOT_SYNCHRONIZED,
      UNDECLARED_COLLATOR,
      PEER_LIMIT_REACHED,
      PROTOCOL_MISMATCH,
      NOT_CONFIRMED,
      NO_STATE,
      NO_SESSION_INFO,
      OUT_OF_BOUND,
      REJECTED_BY_PROSPECTIVE_PARACHAINS,
      INCORRECT_BITFIELD_SIZE,
      CORE_INDEX_UNAVAILABLE,
      INCORRECT_SIGNATURE,
      CLUSTER_TRACKER_ERROR,
      PERSISTED_VALIDATION_DATA_NOT_FOUND,
      PERSISTED_VALIDATION_DATA_MISMATCH,
      CANDIDATE_HASH_MISMATCH,
      PARENT_HEAD_DATA_MISMATCH,
      NO_PEER,
      ALREADY_REQUESTED,
      NOT_ADVERTISED,
      WRONG_PARA,
      THRESHOLD_LIMIT_REACHED,
    };

    virtual ~ParachainProcessor() = default;

    virtual void onValidationProtocolMsg(
        const libp2p::peer::PeerId &peer_id,
        const network::VersionedValidatorProtocolMessage &message) = 0;

    virtual void handle_advertisement(
        const RelayHash &relay_parent,
        const libp2p::peer::PeerId &peer_id,
        std::optional<std::pair<CandidateHash, Hash>>
            &&prospective_candidate) = 0;

    virtual void onIncomingCollator(const libp2p::peer::PeerId &peer_id,
                                    parachain::CollatorPublicKey pubkey,
                                    parachain::ParachainId para_id) = 0;

    virtual outcome::result<void> canProcessParachains() const = 0;

    virtual void handleStatement(
        const primitives::BlockHash &relay_parent,
        const SignedFullStatementWithPVD &statement) = 0;
  };

}  // namespace kagome::parachain

OUTCOME_HPP_DECLARE_ERROR(kagome::parachain, ParachainProcessor::Error);
