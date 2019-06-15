/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
#define KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP

/**
https://github.com/paritytech/substrate/blob/e8739300ae3f7f2e7b72f64668573275f2806ea5/core/rpc/src/author/mod.rs#L50-L49

/// Substrate authoring RPC API
#[rpc]
pub trait AuthorApi<Hash, BlockHash> {
        /// RPC metadata
        type Metadata;

        /// Submit hex-encoded extrinsic for inclusion in block.
        #[rpc(name = "author_submitExtrinsic")]
        fn submit_extrinsic(&self, extrinsic: Bytes) -> Result<Hash>;

        /// Returns all pending extrinsics, potentially grouped by sender.
        #[rpc(name = "author_pendingExtrinsics")]
        fn pending_extrinsics(&self) -> Result<Vec<Bytes>>;

        /// Remove given extrinsic from the pool and temporarily ban it to
prevent reimporting.
        #[rpc(name = "author_removeExtrinsic")]
        fn remove_extrinsic(&self, bytes_or_hash:
Vec<hash::ExtrinsicOrHash<Hash>>) -> Result<Vec<Hash>>;

        /// Submit an extrinsic to watch.
        #[pubsub(subscription = "author_extrinsicUpdate", subscribe, name =
"author_submitAndWatchExtrinsic")] fn watch_extrinsic(&self, metadata:
Self::Metadata, subscriber: Subscriber<Status<Hash, BlockHash>>, bytes: Bytes);

        /// Unsubscribe from extrinsic watching.
        #[pubsub(subscription = "author_extrinsicUpdate", unsubscribe, name =
"author_unwatchExtrinsic")] fn unwatch_extrinsic(&self, metadata:
Option<Self::Metadata>, id: SubscriptionId) -> Result<bool>;
}
 */

#include "runtime/tagged_transaction_queue.hpp"
#include "transaction_pool/transaction_pool.hpp"

namespace kagome::primitives {
  class Metadata;
  class Subscriber;
  class SubscriptionId;

}  // namespace kagome::primitives

namespace kagome::service {
  class ExtrinsicSubmissionService {
   public:
    ExtrinsicSubmissionService(
        std::shared_ptr<runtime::TaggedTransactionQueue> api,
        std::shared_ptr<transaction_pool::TransactionPool> pool);

    outcome::result<common::Hash256> submit_extrinsic(
        const common::Buffer &bytes);

    outcome::result<std::vector<common::Buffer>> pending_extrinsics();

    outcome::result<std::vector<common::Hash256>> remove_extrinsic(
        const std::vector<boost::variant<common::Buffer, common::Hash256>>
            &bytes_or_hash);

    void watch_extrinsic(const primitives::Metadata &metadata,
                         const primitives::Subscriber &subscriber,
                         const common::Buffer);

    outcome::result<bool> unwatch_extrinsic(
        const std::optional<primitives::Metadata> &metadata,
        const primitives::SubscriptionId &id);
  };

}  // namespace kagome::service

#endif  // KAGOME_CORE_SERVICE_EXTRINSICS_SUBMISSION_SERVICE_HPP
