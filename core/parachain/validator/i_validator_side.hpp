#pragma once

#include <optional>
#include <unordered_map>
#include <vector>

#include "primitives/common.hpp"
namespace kagome::parachain {

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
   * @brief Interface for validator-side logic of the collator protocol
   *
   * This interface defines the contract for validator-side state and logic for
   * the collator protocol, particularly focusing on ensuring fair collation
   * fetching across parachains.
   */
  class ValidatorSide {
   public:
    using RelayHash = primitives::BlockHash;
    using CandidateHash = primitives::BlockHash;
    using Hash = primitives::BlockHash;
    using FetchedCandidatesMap = std::unordered_map<network::FetchedCollation,
                                                    network::CollationEvent,
                                                    FetchedCollationHasher>;

    virtual ~ValidatorSide() = default;

    /**
     * @brief Update active leaves and ensure fairness
     *
     * @param active_leaves The new active leaves
     * @param implicit_view The implicit view
     */
    virtual void updateActiveLeaves(
        const std::unordered_map<Hash, ActiveLeafState> &active_leaves,
        const ImplicitView &implicit_view) = 0;

    /**
     * @brief Check if an advertisement can be processed based on claim queue
     * state
     *
     * @param relay_parent The relay parent hash
     * @param para_id The parachain ID
     * @param claim_queue The current claim queue state
     * @return true if the advertisement can be processed
     */
    virtual bool canProcessAdvertisement(
        const RelayHash &relay_parent,
        const ParachainId &para_id,
        const runtime::ClaimQueueSnapshot &claim_queue) = 0;

    /**
     * @brief Register a collation as being fetched
     *
     * @param relay_parent The relay parent hash
     * @param para_id The parachain ID
     */
    virtual void registerCollationFetch(const RelayHash &relay_parent,
                                        const ParachainId &para_id) = 0;

    /**
     * @brief Complete a collation fetch
     *
     * @param relay_parent The relay parent hash
     * @param para_id The parachain ID
     */
    virtual void completeCollationFetch(const RelayHash &relay_parent,
                                        const ParachainId &para_id) = 0;

    /**
     * @brief Get the next collation to fetch based on fair allocation
     *
     * @param relay_parent The relay parent hash
     * @param claim_queue The current claim queue
     * @return std::optional<std::pair<CollatorId, ParachainId>> The next
     * collator and parachain
     */
    virtual std::optional<std::pair<kagome::crypto::Sr25519PublicKey,
                                    std::optional<CandidateHash>>>
    getNextCollationToFetch(
        const RelayHash &relay_parent,
        const std::pair<kagome::crypto::Sr25519PublicKey,
                        std::optional<CandidateHash>> &previous_fetch) = 0;

    /**
     * @brief Add a new fetched candidate
     */
    virtual void addFetchedCandidate(const network::FetchedCollation &collation,
                                     const network::CollationEvent &event) = 0;

    /**
     * @brief Remove a fetched candidate
     */
    virtual void removeFetchedCandidate(
        const network::FetchedCollation &collation) = 0;

    /**
     * @brief Block a collation from seconding
     */
    virtual void blockFromSeconding(const BlockedCollationId &id,
                                    network::PendingCollationFetch &&fetch) = 0;

    /**
     * @brief Get and remove blocked collations for a given ID
     */
    virtual std::vector<network::PendingCollationFetch> takeBlockedCollations(
        const BlockedCollationId &id) = 0;

    /**
     * @brief Access active leaves for modification
     */
    virtual std::unordered_map<Hash, ActiveLeafState> &activeLeaves() = 0;

    /**
     * @brief Check if there are any blocked collations for a given ID
     */
    virtual bool hasBlockedCollations(const BlockedCollationId &id) const = 0;

    /**
     * @brief Access fetched candidates
     */
    virtual const FetchedCandidatesMap &fetchedCandidates() const = 0;

    /**
     * @brief Access fetched candidates for modification
     */
    virtual FetchedCandidatesMap &fetchedCandidates() = 0;
  };

}  // namespace kagome::parachain
