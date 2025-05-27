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
#include "parachain/validator/i_validator_side.hpp"
#include "primitives/common.hpp"

namespace kagome::network {
  struct FetchedCollation;
  struct CollationEvent;
  struct PendingCollationFetch;
  using CollatorPublicKey = libp2p::crypto::PublicKey;
}  // namespace kagome::network

namespace kagome::parachain {
  /**
   * @brief Handles the validator-side logic of the collator protocol
   *
   * This class encapsulates the validator-side state and logic for the collator
   * protocol, particularly focusing on ensuring fair collation fetching across
   * parachains.
   */
  class ValidatorSideImpl : public ValidatorSide {
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

    ValidatorSideImpl();
    ~ValidatorSideImpl() override = default;

    /**
     * @brief Update active leaves and ensure fairness
     *
     * @param active_leaves The new active leaves
     * @param implicit_view The implicit view
     */
    void updateActiveLeaves(
        const std::unordered_map<Hash, ActiveLeafState> &active_leaves,
        const ImplicitView &implicit_view) override;

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
        const runtime::ClaimQueueSnapshot &claim_queue) override;

    /**
     * @brief Register a collation as being fetched
     *
     * @param relay_parent The relay parent hash
     * @param para_id The parachain ID
     */
    void registerCollationFetch(const RelayHash &relay_parent,
                                const ParachainId &para_id) override;

    /**
     * @brief Complete a collation fetch
     *
     * @param relay_parent The relay parent hash
     * @param para_id The parachain ID
     */
    void completeCollationFetch(const RelayHash &relay_parent,
                                const ParachainId &para_id) override;

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
                        std::optional<CandidateHash>> &previous_fetch) override;

    /**
     * @brief Add a new fetched candidate
     */
    void addFetchedCandidate(const network::FetchedCollation &collation,
                             const network::CollationEvent &event) override;

    /**
     * @brief Remove a fetched candidate
     */
    void removeFetchedCandidate(
        const network::FetchedCollation &collation) override;

    /**
     * @brief Block a collation from seconding
     */
    void blockFromSeconding(const BlockedCollationId &id,
                            network::PendingCollationFetch &&fetch) override;

    /**
     * @brief Get and remove blocked collations for a given ID
     */
    std::vector<network::PendingCollationFetch> takeBlockedCollations(
        const BlockedCollationId &id) override;

    /**
     * @brief Check if there are any blocked collations for a given ID
     */
    bool hasBlockedCollations(const BlockedCollationId &id) const override;

    /**
     * @brief Access fetched candidates
     */
    const FetchedCandidatesMap &fetchedCandidates() const override;

    /**
     * @brief Access fetched candidates for modification
     */
    FetchedCandidatesMap &fetchedCandidates() override;

    /**
     * @brief Access active leaves for modification
     */
    std::unordered_map<Hash, ActiveLeafState> &activeLeaves() override;

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

    std::unordered_map<Hash, ActiveLeafState> active_leaves;
  };

}  // namespace kagome::parachain
