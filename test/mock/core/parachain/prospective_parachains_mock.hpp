/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "parachain/validator/prospective_parachains/prospective_parachains.hpp"

#include <gmock/gmock.h>

namespace kagome::parachain {

  class ProspectiveParachainsMock : public IProspectiveParachains {
   public:
    MOCK_METHOD(void, printStoragesLoad, (), (override));

    MOCK_METHOD(std::shared_ptr<blockchain::BlockTree>,
                getBlockTree,
                (),
                (override));

    MOCK_METHOD((std::vector<std::pair<ParachainId, BlockNumber>>),
                answerMinimumRelayParentsRequest,
                (const RelayHash &),
                (override));

    MOCK_METHOD(
        (std::vector<std::pair<CandidateHash, Hash>>),
        answerGetBackableCandidates,
        (const RelayHash &, ParachainId, uint32_t, const fragment::Ancestors &),
        (override));

    MOCK_METHOD(
        outcome::result<std::optional<runtime::PersistedValidationData>>,
        answerProspectiveValidationDataRequest,
        (const RelayHash &, const ParentHeadData &, ParachainId),
        (override));

    MOCK_METHOD(std::optional<ProspectiveParachainsMode>,
                prospectiveParachainsMode,
                (const RelayHash &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                onActiveLeavesUpdate,
                (const network::ExViewRef &),
                (override));

    std::vector<
        std::pair<HypotheticalCandidate, fragment::HypotheticalMembership>>
    answer_hypothetical_membership_request(
        const std::span<const HypotheticalCandidate> &candidates,
        const std::optional<std::reference_wrapper<const Hash>>
            &fragment_tree_relay_parent) override {
      IProspectiveParachains::HypotheticalMembershipRequest request;
      for (const auto &c : candidates) {
        request.candidates.emplace_back(c);
      }
      if (fragment_tree_relay_parent) {
        request.fragment_chain_relay_parent = fragment_tree_relay_parent->get();
      }
      return answer_hypothetical_membership_request(request);
    }

    MOCK_METHOD((std::vector<std::pair<HypotheticalCandidate,
                                       fragment::HypotheticalMembership>>),
                answer_hypothetical_membership_request,
                (const IProspectiveParachains::HypotheticalMembershipRequest &),
                ());

    MOCK_METHOD(void,
                candidate_backed,
                (ParachainId, const CandidateHash &),
                (override));

    MOCK_METHOD(bool,
                introduce_seconded_candidate,
                (ParachainId,
                 const network::CommittedCandidateReceipt &,
                 (const crypto::Hashed<runtime::PersistedValidationData,
                                       32,
                                       crypto::Blake2b_StreamHasher<32>> &),
                 const CandidateHash &),
                (override));
  };

}  // namespace kagome::parachain

namespace boost {
  inline auto &operator<<(std::ostream &s,
                          const kagome::parachain::ParentHeadData &) {
    return s;
  }
  inline auto &operator<<(std::ostream &s,
                          const kagome::parachain::HypotheticalCandidate &) {
    return s;
  }
}  // namespace boost
