/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_SERVICE_AUTHOR_IMPL_AUTHOR_API_IMPL_HPP
#define KAGOME_CORE_API_SERVICE_AUTHOR_IMPL_AUTHOR_API_IMPL_HPP

/**
 * ExtrinsicSubmissionApi based on auth api implemented in substrate here
 * https://github.com/paritytech/substrate/blob/e8739300ae3f7f2e7b72f64668573275f2806ea5/core/rpc/src/author/mod.rs#L50-L49
 */

#include <unordered_set>

#include <libp2p/peer/peer_id.hpp>

#include "api/service/author/author_api.hpp"
#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "crypto/hasher.hpp"
#include "network/extrinsic_gossiper.hpp"
#include "outcome/outcome.hpp"
#include "storage/trie/trie_storage.hpp"

namespace kagome::transaction_pool {
  class TransactionPool;
}

namespace kagome::runtime {
  class TaggedTransactionQueue;
}

namespace kagome::subscription {
  template <typename Event, typename Receiver, typename... Arguments>
  class Subscriber;
}

namespace kagome::api {

  class AuthorApiImpl : public AuthorApi {
    template <class T>
    using sptr = std::shared_ptr<T>;
    template <class T>
    using uptr = std::unique_ptr<T>;

   public:
    /**
     * @constructor
     * @param api ttq instance shared ptr
     * @param pool transaction pool instance shared ptr
     * @param hasher hasher instance shared ptr
     * @param block_tree block tree instance shared ptr
     */
    AuthorApiImpl(std::shared_ptr<runtime::TaggedTransactionQueue> api,
                  std::shared_ptr<transaction_pool::TransactionPool> pool,
                  std::shared_ptr<crypto::Hasher> hasher,
                  std::shared_ptr<network::ExtrinsicGossiper> gossiper);

    ~AuthorApiImpl() override = default;

    outcome::result<common::Hash256> submitExtrinsic(
        const primitives::Extrinsic &extrinsic) override;

    outcome::result<std::vector<primitives::Extrinsic>> pendingExtrinsics()
        override;

    outcome::result<std::vector<common::Hash256>> removeExtrinsic(
        const std::vector<primitives::ExtrinsicKey> &keys) override;

    outcome::result<SubscriptionId> submitAndWatchExtrinsic(
        Extrinsic extrinsic) override;

    outcome::result<bool> unwatchExtrinsic(
        SubscriptionId subscription_id) override;

   private:
    /// need this in addition to sub id in extrinsics themselves because
    /// currently extrinsics can be copied and it is thus impossible to
    /// unsubscribe from them directly
    std::unordered_set<SubscriptionId> subscribed_ids_;

    sptr<runtime::TaggedTransactionQueue> api_;
    sptr<transaction_pool::TransactionPool> pool_;
    sptr<crypto::Hasher> hasher_;
    sptr<blockchain::BlockTree> block_tree_;
    sptr<network::ExtrinsicGossiper> gossiper_;

    std::atomic<primitives::ObservedExtrinsicId> latest_id_ = 0;

    common::Logger logger_;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_SERVICE_AUTHOR_IMPL_AUTHOR_API_IMPL_HPP
