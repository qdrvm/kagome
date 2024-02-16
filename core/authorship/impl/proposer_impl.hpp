/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "authorship/proposer.hpp"

#include "authorship/block_builder_factory.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "runtime/runtime_api/block_builder.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::authorship {

  /**
   * @class ProposerImpl
   * @file proposer_impl.hpp
   * @brief The ProposerImpl class is responsible for proposing a new block for
   * the blockchain.
   *
   * It uses a BlockBuilderFactory to create new blocks, a Clock to keep track
   * of time, a TransactionPool to manage transactions, an
   * ExtrinsicSubscriptionEngine to handle extrinsic events, and an
   * ExtrinsicEventKeyRepository to manage event keys.
   *
   * @see BlockBuilderFactory
   * @see Clock
   * @see transaction_pool::TransactionPool
   * @see primitives::events::ExtrinsicSubscriptionEngine
   * @see subscription::ExtrinsicEventKeyRepository
   */
  class ProposerImpl : public Proposer {
   public:
    /// Maximum transactions quantity to try to push into the block before
    /// finalization when resources are exhausted (block size limit reached)
    static constexpr auto kMaxSkippedTransactions = 8;

    /// Default block size limit in bytes
    static constexpr size_t kBlockSizeLimit = 4 * 1024 * 1024 + 512;

    ~ProposerImpl() override = default;

    ProposerImpl(
        std::shared_ptr<BlockBuilderFactory> block_builder_factory,
        std::shared_ptr<Clock> clock,
        std::shared_ptr<transaction_pool::TransactionPool> transaction_pool,
        std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
            ext_sub_engine,
        std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
            extrinsic_event_key_repo);

    /**
     * @brief Proposes a new block for the blockchain.
     *
     * This method uses the current state of the transaction pool to propose a
     * new block. It selects transactions from the pool, creates a new block
     * with these transactions, and returns the new block.
     *
     * @param parent_block The parent block of the new block.
     * @param deadline The deadline for the new block.
     * @param inherent_data The inherent data to be included in the new block.
     * @param digest The digest to be included in the new block.
     * @return A block containing the proposed transactions.
     *
     * This method performs the following steps:
     * 1. Creates a new block builder.
     * 2. Retrieves and adds the inherent extrinsics to the block.
     * 3. Removes stale transactions from the transaction pool.
     * 4. Retrieves ready transactions from the transaction pool.
     * 5. Adds transactions to the block until the block size limit is reached
     * or the deadline is met.
     * 6. Finalizes the block construction and returns the built block.
     * 7. Removes the included transactions from the transaction pool.
     */
    outcome::result<primitives::Block> propose(
        const primitives::BlockInfo &parent_block,
        std::optional<Clock::TimePoint> deadline,
        const primitives::InherentData &inherent_data,
        const primitives::Digest &inherent_digest,
        TrieChangesTrackerOpt changes_tracker) override;

   private:
    std::shared_ptr<BlockBuilderFactory> block_builder_factory_;
    std::shared_ptr<Clock> clock_;
    std::shared_ptr<transaction_pool::TransactionPool> transaction_pool_;
    std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
        ext_sub_engine_;
    std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
        extrinsic_event_key_repo_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_tx_included_in_block_;

    log::Logger logger_ = log::createLogger("Proposer", "authorship");
  };

}  // namespace kagome::authorship
