/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <parachain/validator/backing_implicit_view.hpp>
#include <parachain/validator/statement_distribution/per_relay_parent_state.hpp>
#include "authority_discovery/query/query.hpp"
#include "common/ref_cache.hpp"
#include "consensus/babe/babe_config_repository.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/timeline/slots_util.hpp"
#include "network/can_disconnect.hpp"
#include "network/peer_manager.hpp"
#include "network/peer_view.hpp"
#include "network/router.hpp"
#include "parachain/approval/approval_thread_pool.hpp"
#include "parachain/validator/impl/candidates.hpp"
#include "parachain/validator/network_bridge.hpp"
#include "parachain/validator/signer.hpp"
#include "parachain/validator/statement_distribution/peer_state.hpp"
#include "parachain/validator/statement_distribution/per_session_state.hpp"
#include "parachain/validator/statement_distribution/types.hpp"
#include "utils/pool_handler_ready_make.hpp"

namespace kagome::parachain {
  class ParachainProcessorImpl;
}

namespace kagome::parachain::statement_distribution {

  class IStatementDistribution {
   public:
    virtual ~IStatementDistribution() = default;

    virtual void OnFetchAttestedCandidateRequest(
        const network::vstaging::AttestedCandidateRequest &request,
        std::shared_ptr<libp2p::connection::Stream> stream) = 0;

    virtual void store_parachain_processor(
        std::weak_ptr<ParachainProcessorImpl> pp) = 0;

    virtual void handle_incoming_manifest(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::BackedCandidateManifest &msg) = 0;

    virtual void handle_incoming_acknowledgement(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::BackedCandidateAcknowledgement
            &acknowledgement) = 0;

    virtual void handle_incoming_statement(
        const libp2p::peer::PeerId &peer_id,
        const network::vstaging::StatementDistributionMessageStatement
            &stm) = 0;

    virtual void handle_backed_candidate_message(
        const CandidateHash &candidate_hash) = 0;

    virtual void share_local_statement(
        const primitives::BlockHash &relay_parent,
        const SignedFullStatementWithPVD &statement) = 0;
  };

}  // namespace kagome::parachain::statement_distribution
