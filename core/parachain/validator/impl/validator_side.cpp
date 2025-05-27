#include "parachain/validator/impl/validator_side.hpp"

#include "network/types/collator_messages_vstaging.hpp"
#include "parachain/validator/blocked_collation_id.hpp"
#include "parachain/validator/fetched_collation_hash.hpp"
#include "scale/scale.hpp"

namespace kagome::parachain {

  ValidatorSideImpl::ValidatorSideImpl()
      : active_leaves{},
        fetched_candidates_{std::make_unique<FetchedCandidatesMap>()},
        blocked_from_seconding_{},
        claim_queue_state_{} {}

  void ValidatorSideImpl::updateActiveLeaves(
      const std::unordered_map<Hash, ActiveLeafState> &active_leaves,
      const ImplicitView &implicit_view) {
    this->active_leaves = active_leaves;
  }

  bool ValidatorSideImpl::canProcessAdvertisement(
      const RelayHash &relay_parent,
      const ParachainId &para_id,
      const runtime::ClaimQueueSnapshot &claim_queue) {
    // Update claim queue state with the latest information
    claim_queue_state_.updateClaimQueue(relay_parent, claim_queue);

    // Check if the parachain can claim at this relay parent
    return claim_queue_state_.canClaimAt(relay_parent, para_id);
  }

  void ValidatorSideImpl::registerCollationFetch(const RelayHash &relay_parent,
                                                 const ParachainId &para_id) {
    // Call ClaimQueueState's method
    claim_queue_state_.registerFetchAttempt(relay_parent, para_id);
  }

  void ValidatorSideImpl::completeCollationFetch(const RelayHash &relay_parent,
                                                 const ParachainId &para_id) {
    // Call ClaimQueueState's method
    claim_queue_state_.completeFetchAttempt(relay_parent, para_id);
  }

  std::optional<std::pair<kagome::crypto::Sr25519PublicKey,
                          std::optional<ValidatorSideImpl::CandidateHash>>>
  ValidatorSideImpl::getNextCollationToFetch(
      const RelayHash &relay_parent,
      const std::pair<kagome::crypto::Sr25519PublicKey,
                      std::optional<CandidateHash>> &previous_fetch) {
    // Check if the relay parent exists in active leaves
    auto it = active_leaves.find(relay_parent);
    if (it == active_leaves.end()) {
      // Relay parent is not active anymore
      return std::nullopt;
    }

    // Get all the parachains with registered claims in the queue
    std::vector<ParachainId> candidate_paras;
    auto &relay_parent_claims =
        claim_queue_state_.state_by_relay_parent_and_para_[relay_parent];

    for (const auto &[para_id, claim_state] : relay_parent_claims) {
      // Select parachains that have claims but aren't fully fetched yet
      if (claim_state.num_claims > claim_state.num_active) {
        candidate_paras.push_back(para_id);
      }
    }

    if (candidate_paras.empty()) {
      // No parachains with available claims
      return std::nullopt;
    }

    // Simple round-robin: choose the first parachain that isn't associated with
    // the previous collator
    ParachainId selected_para = candidate_paras[0];

    // Find a collator for this parachain in our fetched candidates
    std::optional<kagome::crypto::Sr25519PublicKey> next_collator =
        std::nullopt;
    std::optional<CandidateHash> candidate_hash = std::nullopt;

    // Find a candidate from a different collator than the previous one
    for (const auto &[fetched_collation, event] : *fetched_candidates_) {
      if (fetched_collation.relay_parent == relay_parent
          && fetched_collation.para_id == selected_para) {
        // Extract collator ID from the event
        auto collator_id = event.collator_id;

        // Skip if this is the same collator as the previous fetch
        if (previous_fetch.first == collator_id) {
          continue;
        }

        next_collator = collator_id;
        candidate_hash = fetched_collation.candidate_hash;
        break;
      }
    }

    // If no suitable collator found in fetched candidates, return null
    if (!next_collator.has_value()) {
      return std::nullopt;
    }

    // Return the collator public key and optional candidate hash
    return std::make_pair(next_collator.value(), candidate_hash);
  }

  void ValidatorSideImpl::addFetchedCandidate(
      const network::FetchedCollation &collation,
      const network::CollationEvent &event) {
    // Add to the fetched candidates map using insert instead of operator[]
    // to avoid requiring default constructibility of CollationEvent
    fetched_candidates_->insert_or_assign(collation, event);
  }

  void ValidatorSideImpl::removeFetchedCandidate(
      const network::FetchedCollation &collation) {
    // Remove from the fetched candidates map
    fetched_candidates_->erase(collation);
  }

  void ValidatorSideImpl::blockFromSeconding(
      const BlockedCollationId &id, network::PendingCollationFetch &&fetch) {
    // Add to the blocked collations map
    blocked_from_seconding_[id].push_back(std::move(fetch));
  }

  std::vector<network::PendingCollationFetch>
  ValidatorSideImpl::takeBlockedCollations(const BlockedCollationId &id) {
    // Get and remove from the blocked collations map
    std::vector<network::PendingCollationFetch> result;

    auto it = blocked_from_seconding_.find(id);
    if (it != blocked_from_seconding_.end()) {
      result = std::move(it->second);
      blocked_from_seconding_.erase(it);
    }

    return result;
  }

  std::unordered_map<Hash, ActiveLeafState> &ValidatorSideImpl::activeLeaves() {
    return active_leaves;
  }

  bool ValidatorSideImpl::hasBlockedCollations(
      const BlockedCollationId &id) const {
    return blocked_from_seconding_.count(id) > 0;
  }

  const ValidatorSideImpl::FetchedCandidatesMap &
  ValidatorSideImpl::fetchedCandidates() const {
    return *fetched_candidates_;
  }

  ValidatorSideImpl::FetchedCandidatesMap &
  ValidatorSideImpl::fetchedCandidates() {
    return *fetched_candidates_;
  }

}  // namespace kagome::parachain
