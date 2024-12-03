/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "parachain/validator/statement_distribution/i_statement_distribution.hpp"

namespace kagome::parachain::statement_distribution {

  class StatementDistributionMock : public IStatementDistribution {
   public:
    MOCK_METHOD(void,
                OnFetchAttestedCandidateRequest,
                (const network::vstaging::AttestedCandidateRequest &, std::shared_ptr<libp2p::connection::Stream>),
                (override));

    MOCK_METHOD(void,
                store_parachain_processor,
                (std::weak_ptr<ParachainProcessorImpl>),
                (override));

    MOCK_METHOD(void,
                handle_incoming_manifest,
                (const libp2p::peer::PeerId &, const network::vstaging::BackedCandidateManifest &),
                (override));

    MOCK_METHOD(void,
                handle_incoming_acknowledgement,
                (const libp2p::peer::PeerId &, const network::vstaging::BackedCandidateAcknowledgement &),
                (override));

    MOCK_METHOD(void,
                handle_incoming_statement,
                (const libp2p::peer::PeerId &, const network::vstaging::StatementDistributionMessageStatement &),
                (override));

    MOCK_METHOD(void,
                handle_backed_candidate_message,
                (const CandidateHash &),
                (override));

    MOCK_METHOD(void,
                share_local_statement,
                (const primitives::BlockHash &, const SignedFullStatementWithPVD &),
                (override));
  };

}  // namespace kagome::parachain::statement_distribution
