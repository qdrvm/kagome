/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>
#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "network/peer_view.hpp"
#include "network/types/collator_messages_vstaging.hpp"
#include "parachain/types.hpp"
#include "parachain/validator/backing_implicit_view.hpp"
#include "parachain/validator/collations.hpp"
#include "parachain/validator/prospective_parachains/fragment_chain.hpp"
#include "parachain/validator/prospective_parachains/fragment_chain_errors.hpp"
#include "runtime/runtime_api/parachain_host.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"
#include "utils/map.hpp"

namespace kagome::parachain {

  using ParentHeadData_OnlyHash = Hash;
  using ParentHeadData_WithData = std::pair<HeadData, Hash>;
  using ParentHeadData =
      boost::variant<ParentHeadData_OnlyHash, ParentHeadData_WithData>;

  class IProspectiveParachains {
   public:
   struct HypotheticalMembershipRequest {
	std::vector<HypotheticalCandidate> candidates;
	std::optional<Hash> fragment_chain_relay_parent;

    bool operator==(const HypotheticalMembershipRequest &other) const = default;
    // {
    //     return candidates == other.candidates && fragment_chain_relay_parent == other.fragment_chain_relay_parent;
    // }
};

    virtual ~IProspectiveParachains() = default;

    // Debug print of all internal buffers load.
    virtual void printStoragesLoad() = 0;

    virtual std::shared_ptr<blockchain::BlockTree> getBlockTree() = 0;

    virtual std::vector<std::pair<ParachainId, BlockNumber>>
    answerMinimumRelayParentsRequest(const RelayHash &relay_parent) = 0;

    virtual std::vector<std::pair<CandidateHash, Hash>>
    answerGetBackableCandidates(const RelayHash &relay_parent,
                                ParachainId para,
                                uint32_t count,
                                const fragment::Ancestors &ancestors) = 0;

    virtual outcome::result<std::optional<runtime::PersistedValidationData>>
    answerProspectiveValidationDataRequest(
        const RelayHash &candidate_relay_parent,
        const ParentHeadData &parent_head_data,
        ParachainId para_id) = 0;

    virtual std::optional<ProspectiveParachainsMode> prospectiveParachainsMode(
        const RelayHash &relay_parent) = 0;

    virtual outcome::result<void> onActiveLeavesUpdate(
        const network::ExViewRef &update) = 0;

//    virtual std::vector<
//        std::pair<HypotheticalCandidate, fragment::HypotheticalMembership>>
//    answer_hypothetical_membership_request(
//        const HypotheticalMembershipRequest &request) = 0;

    virtual std::vector<
        std::pair<HypotheticalCandidate, fragment::HypotheticalMembership>>
    answer_hypothetical_membership_request(
        const std::span<const HypotheticalCandidate> &candidates,
        const std::optional<std::reference_wrapper<const Hash>>
            &fragment_tree_relay_parent) = 0;

    virtual void candidate_backed(ParachainId para,
                                  const CandidateHash &candidate_hash) = 0;

    virtual bool introduce_seconded_candidate(
        ParachainId para,
        const network::CommittedCandidateReceipt &candidate,
        const crypto::Hashed<runtime::PersistedValidationData,
                             32,
                             crypto::Blake2b_StreamHasher<32>> &pvd,
        const CandidateHash &candidate_hash) = 0;
  };

  class ProspectiveParachains
      : public IProspectiveParachains,
        public std::enable_shared_from_this<ProspectiveParachains> {
#ifdef CFG_TESTING
   public:
#endif  // CFG_TESTING
    struct RelayBlockViewData {
      // The fragment chains for current and upcoming scheduled paras.
      std::unordered_map<ParachainId, fragment::FragmentChain> fragment_chains;
    };

    struct View {
      // Per relay parent fragment chains. These includes all relay parents
      // under the implicit view.
      std::unordered_map<Hash, RelayBlockViewData> per_relay_parent;
      // The hashes of the currently active leaves. This is a subset of the keys
      // in `per_relay_parent`.
      std::unordered_set<Hash> active_leaves;
      // The backing implicit view.
      ImplicitView implicit_view;

      // Get the fragment chains of this leaf.
      std::optional<std::reference_wrapper<
          const std::unordered_map<ParachainId, fragment::FragmentChain>>>
      get_fragment_chains(const Hash &leaf) const {
        auto view_data = utils::get(per_relay_parent, leaf);
        if (view_data) {
          return std::cref(view_data->get().fragment_chains);
        }
        return std::nullopt;
      }
    };

    struct ImportablePendingAvailability {
      network::CommittedCandidateReceipt candidate;
      runtime::PersistedValidationData persisted_validation_data;
      fragment::PendingAvailability compact;
    };

    std::optional<View> view_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<runtime::ParachainHost> parachain_host_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    log::Logger logger =
        log::createLogger("ProspectiveParachains", "parachain");

    View &view();

   public:
    ProspectiveParachains(
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<runtime::ParachainHost> parachain_host,
        std::shared_ptr<blockchain::BlockTree> block_tree);

    // Debug print of all internal buffers load.
    void printStoragesLoad() override;

    std::shared_ptr<blockchain::BlockTree> getBlockTree() override;

    std::vector<std::pair<ParachainId, BlockNumber>>
    answerMinimumRelayParentsRequest(const RelayHash &relay_parent) override;

    std::vector<std::pair<CandidateHash, Hash>> answerGetBackableCandidates(
        const RelayHash &relay_parent,
        ParachainId para,
        uint32_t count,
        const fragment::Ancestors &ancestors) override;

    outcome::result<std::optional<runtime::PersistedValidationData>>
    answerProspectiveValidationDataRequest(
        const RelayHash &candidate_relay_parent,
        const ParentHeadData &parent_head_data,
        ParachainId para_id) override;

    std::optional<ProspectiveParachainsMode> prospectiveParachainsMode(
        const RelayHash &relay_parent) override;

    outcome::result<std::optional<
        std::pair<fragment::Constraints,
                  std::vector<fragment::CandidatePendingAvailability>>>>
    fetchBackingState(const RelayHash &relay_parent, ParachainId para_id);

    outcome::result<std::optional<fragment::BlockInfoProspectiveParachains>>
    fetchBlockInfo(const RelayHash &relay_hash);

    outcome::result<std::unordered_set<ParachainId>> fetchUpcomingParas(
        const RelayHash &relay_parent,
        std::unordered_set<CandidateHash> &pending_availability);

    outcome::result<std::vector<fragment::BlockInfoProspectiveParachains>>
    fetchAncestry(const RelayHash &relay_hash, size_t ancestors);

    outcome::result<std::vector<ImportablePendingAvailability>>
    preprocessCandidatesPendingAvailability(
        const HeadData &required_parent,
        const std::vector<fragment::CandidatePendingAvailability>
            &pending_availability);

    outcome::result<void> onActiveLeavesUpdate(
        const network::ExViewRef &update) override;

    /// @brief calculates hypothetical candidate and fragment tree membership
    /// @param candidates Candidates, in arbitrary order, which should be
    /// checked for possible membership in fragment trees.
    /// @param fragment_tree_relay_parent Either a specific fragment tree to
    /// check, otherwise all.
    /// @param backed_in_path_only Only return membership if all candidates in
    /// the path from the root are backed.
    std::vector<
        std::pair<HypotheticalCandidate, fragment::HypotheticalMembership>>
    answer_hypothetical_membership_request(
        const std::span<const HypotheticalCandidate> &candidates,
        const std::optional<std::reference_wrapper<const Hash>>
            &fragment_tree_relay_parent) override;

    // std::vector<
    //     std::pair<HypotheticalCandidate, fragment::HypotheticalMembership>>
    // answer_hypothetical_membership_request(
    //     const HypotheticalMembershipRequest &request) override;


    void candidate_backed(ParachainId para,
                          const CandidateHash &candidate_hash) override;

    bool introduce_seconded_candidate(
        ParachainId para,
        const network::CommittedCandidateReceipt &candidate,
        const crypto::Hashed<runtime::PersistedValidationData,
                             32,
                             crypto::Blake2b_StreamHasher<32>> &pvd,
        const CandidateHash &candidate_hash) override;
  };

}  // namespace kagome::parachain
