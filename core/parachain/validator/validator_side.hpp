#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "libp2p/peer/peer_id.hpp"
#include "parachain/types.hpp"
#include "parachain/validator/backing_implicit_view.hpp"
#include "parachain/validator/blocked_collation_id_hash.hpp"
#include "parachain/validator/claim_queue_state.hpp"
#include "parachain/validator/collations.hpp"
#include "parachain/validator/fetched_collation_hash.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  struct FetchedCollation;
  struct CollationEvent;
  struct PendingCollationFetch;
  using CollatorPublicKey = libp2p::crypto::PublicKey;
}  // namespace kagome::network

namespace kagome::parachain {

  // Forward declarations
  struct BlockedCollationId;

  // Custom hash function for FetchedCollation
  struct FetchedCollationHasher {
    size_t operator()(const network::FetchedCollation &value) const {
      size_t h1 = std::hash<primitives::BlockHash>{}(value.relay_parent);
      size_t h2 = std::hash<parachain::ParachainId>{}(value.para_id);
      size_t h3 = std::hash<primitives::BlockHash>{}(value.candidate_hash);
      return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
  };

  /**
   * @brief Handles the validator-side logic of the collator protocol
   *
   * This class encapsulates the validator-side state and logic for the collator
   * protocol, particularly focusing on ensuring fair collation fetching across
   * parachains.
   */
  class ValidatorSide {
   public:
    using RelayHash = primitives::BlockHash;
    using CandidateHash = primitives::BlockHash;
    using Hash = primitives::BlockHash;
    using ParachainId = parachain::ParachainId;
    using PeerId = libp2p::peer::PeerId;
    using ProspectiveParachainsModeOpt =
        std::optional<ProspectiveParachainsMode>;
    using CollatorId = network::CollatorPublicKey;
    using FetchedCandidatesMap = std::unordered_map<network::FetchedCollation,
                                                    network::CollationEvent,
                                                    FetchedCollationHasher>;

    ValidatorSide();
    ~ValidatorSide() = default;

    /**
     * @brief Update active leaves and ensure fairness
     *
     * @param active_leaves The new active leaves
     * @param implicit_view The implicit view
     */
    void updateActiveLeaves(
        const std::unordered_map<Hash, ActiveLeafState> &active_leaves,
        const ImplicitView &implicit_view);

    /**
     * @brief Check if an advertisement can be processed based on claim queue
     * state
     *
     * @param relay_parent The relay parent hash
     * @param para_id The parachain ID
     * @param claim_queue The current claim queue state
     * @return true if the advertisement can be processed
     */
    bool canProcessAdvertisement(
        const RelayHash &relay_parent,
        const ParachainId &para_id,
        const runtime::ClaimQueueSnapshot &claim_queue);

    /**
     * @brief Register a collation as being fetched
     *
     * @param relay_parent The relay parent hash
     * @param para_id The parachain ID
     */
    void registerCollationFetch(const RelayHash &relay_parent,
                                const ParachainId &para_id);

    /**
     * @brief Complete a collation fetch
     *
     * @param relay_parent The relay parent hash
     * @param para_id The parachain ID
     */
    void completeCollationFetch(const RelayHash &relay_parent,
                                const ParachainId &para_id);

    /**
     * @brief Get the next collation to fetch based on fair allocation
     *
     * @param relay_parent The relay parent hash
     * @param claim_queue The current claim queue
     * @return std::optional<std::pair<CollatorId, ParachainId>> The next
     * collator and parachain
     */
    std::optional<std::pair<kagome::crypto::Sr25519PublicKey,
                            std::optional<CandidateHash>>>
    getNextCollationToFetch(
        const RelayHash &relay_parent,
        const std::pair<kagome::crypto::Sr25519PublicKey,
                        std::optional<CandidateHash>> &previous_fetch);

    /**
     * @brief Add a new fetched candidate
     */
    void addFetchedCandidate(const network::FetchedCollation &collation,
                             const network::CollationEvent &event);

    /**
     * @brief Remove a fetched candidate
     */
    void removeFetchedCandidate(const network::FetchedCollation &collation);

    /**
     * @brief Block a collation from seconding
     */
    void blockFromSeconding(const BlockedCollationId &id,
                            network::PendingCollationFetch &&fetch);

    /**
     * @brief Get and remove blocked collations for a given ID
     */
    std::vector<network::PendingCollationFetch> takeBlockedCollations(
        const BlockedCollationId &id);

    /**
     * @brief Check if there are any blocked collations for a given ID
     */
    bool hasBlockedCollations(const BlockedCollationId &id) const;

    /**
     * @brief Access fetched candidates
     */
    const FetchedCandidatesMap &fetchedCandidates() const {
      return *fetched_candidates_;
    }

    /**
     * @brief Access fetched candidates for modification
     */
    FetchedCandidatesMap &fetchedCandidates() {
      return *fetched_candidates_;
    }

    // Active leaves with active leaf state - made public for direct access
    std::unordered_map<Hash, ActiveLeafState> active_leaves;

   private:
    // Fetched candidates waiting for validation - using unique_ptr to avoid
    // default constructor requirement
    std::unique_ptr<FetchedCandidatesMap> fetched_candidates_;

    // Collations blocked from seconding (waiting for parent head)
    std::unordered_map<BlockedCollationId,
                       std::vector<network::PendingCollationFetch>>
        blocked_from_seconding_;

    // Claim queue state manager
    ClaimQueueState claim_queue_state_;
  };

}  // namespace kagome::parachain
