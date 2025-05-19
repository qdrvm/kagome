#include "parachain/validator/collations.hpp"

namespace kagome::parachain {

  void Collations::queueCollation(const PendingCollation &collation,
                                  const CollatorId &collator_id) {
    // Add to waiting queue
    auto candidate_hash =
        collation.prospective_candidate.has_value()
            ? std::optional<CandidateHash>(
                  collation.prospective_candidate->candidate_hash)
            : std::nullopt;

    // Use insert or emplace instead of operator[] to avoid default construction
    auto it = waiting_collations_.find(collator_id);
    if (it == waiting_collations_.end()) {
      waiting_collations_.emplace(
          collator_id,
          std::unordered_map<std::optional<CandidateHash>, PendingCollation>{
              {candidate_hash, collation}});
    } else {
      it->second.emplace(candidate_hash, collation);
    }

    // Increment count for this para at this relay parent
    para_counts_[collation.relay_parent][collation.para_id]++;
  }

  void Collations::removeCollation(const PendingCollation &collation,
                                   const CollatorId &collator_id) {
    auto candidate_hash =
        collation.prospective_candidate.has_value()
            ? std::optional<CandidateHash>(
                  collation.prospective_candidate->candidate_hash)
            : std::nullopt;

    // Remove from waiting queue
    auto it_collator = waiting_collations_.find(collator_id);
    if (it_collator != waiting_collations_.end()) {
      it_collator->second.erase(candidate_hash);
      if (it_collator->second.empty()) {
        waiting_collations_.erase(it_collator);
      }
    }

    // Decrement count for this para at this relay parent
    auto it_rp = para_counts_.find(collation.relay_parent);
    if (it_rp != para_counts_.end()) {
      auto it_para = it_rp->second.find(collation.para_id);
      if (it_para != it_rp->second.end()) {
        if (it_para->second > 0) {
          it_para->second--;
          if (it_para->second == 0) {
            it_rp->second.erase(it_para);
            if (it_rp->second.empty()) {
              para_counts_.erase(it_rp);
            }
          }
        }
      }
    }
  }

  size_t Collations::numQueuedForPara(
      const primitives::BlockHash &relay_parent,
      const parachain::ParachainId &para_id) const {
    auto it_rp = para_counts_.find(relay_parent);
    if (it_rp == para_counts_.end()) {
      return 0;
    }

    auto it_para = it_rp->second.find(para_id);
    if (it_para == it_rp->second.end()) {
      return 0;
    }

    return it_para->second;
  }

  std::optional<std::reference_wrapper<const PendingCollation>>
  Collations::getPendingCollation(
      const CollatorId &collator_id,
      const std::optional<CandidateHash> &candidate_hash) const {
    auto it_collator = waiting_collations_.find(collator_id);
    if (it_collator == waiting_collations_.end()) {
      return std::nullopt;
    }

    auto it_candidate = it_collator->second.find(candidate_hash);
    if (it_candidate == it_collator->second.end()) {
      return std::nullopt;
    }

    return std::cref(it_candidate->second);
  }

  void Collations::removePendingCollation(const PendingCollation &collation,
                                          const CollatorId &collator_id) {
    removeCollation(collation, collator_id);
  }

  std::vector<parachain::ParachainId> Collations::getAllClaimedParas(
      const primitives::BlockHash &relay_parent) const {
    std::vector<parachain::ParachainId> result;

    auto it_rp = para_counts_.find(relay_parent);
    if (it_rp == para_counts_.end()) {
      return result;
    }

    for (const auto &[para_id, _] : it_rp->second) {
      result.push_back(para_id);
    }

    return result;
  }

}  // namespace kagome::parachain
