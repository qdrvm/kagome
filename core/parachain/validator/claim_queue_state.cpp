#include "parachain/validator/claim_queue_state.hpp"

namespace kagome::parachain {

  void ClaimQueueState::updateClaimQueue(
      const RelayParentHash &relay_parent,
      const runtime::ClaimQueueSnapshot &claim_queue) {
    // Create or get the state map for this relay parent
    auto &relay_parent_map = state_by_relay_parent_and_para_[relay_parent];

    // First, update claim counts for each parachain
    for (const auto &[core_idx, core_claims] : claim_queue.claimes) {
      for (const auto &para_id : core_claims) {
        auto &state = relay_parent_map[para_id];
        state.num_claims++;
      }
    }
  }

  bool ClaimQueueState::canClaimAt(const RelayParentHash &relay_parent,
                                   const ParachainId &para_id) {
    // Get the state for this relay parent and parachain
    auto rp_it = state_by_relay_parent_and_para_.find(relay_parent);
    if (rp_it == state_by_relay_parent_and_para_.end()) {
      // If we have no state for this relay parent, always allow
      return true;
    }

    auto para_it = rp_it->second.find(para_id);
    if (para_it == rp_it->second.end()) {
      // If we have no state for this parachain, allow as well
      return true;
    }

    // The basic fairness rule: a parachain can claim if it has
    // more entries in the claim queue than active fetches
    return para_it->second.num_claims > para_it->second.num_active;
  }

  void ClaimQueueState::registerFetchAttempt(
      const RelayParentHash &relay_parent, const ParachainId &para_id) {
    // Increment active count
    auto &state = state_by_relay_parent_and_para_[relay_parent][para_id];
    state.num_active++;
  }

  void ClaimQueueState::completeFetchAttempt(
      const RelayParentHash &relay_parent, const ParachainId &para_id) {
    // Decrement active count if possible
    auto rp_it = state_by_relay_parent_and_para_.find(relay_parent);
    if (rp_it != state_by_relay_parent_and_para_.end()) {
      auto para_it = rp_it->second.find(para_id);
      if (para_it != rp_it->second.end() && para_it->second.num_active > 0) {
        para_it->second.num_active--;
      }
    }
  }

}  // namespace kagome::parachain
