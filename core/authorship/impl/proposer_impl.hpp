/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_AUTHORSHIP_IMPL_AITHORING_IMPL_HPP
#define KAGOME_CORE_AUTHORSHIP_IMPL_AITHORING_IMPL_HPP

#include "authorship/proposer.hpp"

#include "authorship/block_builder_factory.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "runtime/runtime_api/block_builder.hpp"
#include "subscription/extrinsic_event_key_repository.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::authorship {

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
        std::shared_ptr<transaction_pool::TransactionPool> transaction_pool,
        std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
            ext_sub_engine,
        std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
            extrinsic_event_key_repo);

    outcome::result<primitives::Block> propose(
        const primitives::BlockInfo &parent_block,
        const primitives::InherentData &inherent_data,
        const primitives::Digest &inherent_digest,
        TrieChangesTrackerOpt changes_tracker) override;

   private:
    std::shared_ptr<BlockBuilderFactory> block_builder_factory_;
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

#endif  // KAGOME_CORE_AUTHORSHIP_IMPL_AITHORING_IMPL_HPP
