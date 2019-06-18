/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_HPP

/**
 * ExtrinsicSubmissionApi based on auth api implemented in substrate here
 * https://github.com/paritytech/substrate/blob/e8739300ae3f7f2e7b72f64668573275f2806ea5/core/rpc/src/author/mod.rs#L50-L49
 */

#include <boost/variant.hpp>
#include <outcome/outcome.hpp>
#include "common/visitor.hpp"
#include "crypto/hasher.hpp"
#include "extrinsics_submission_service/error.hpp"
#include "primitives/auth_api.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::transaction_pool {
  class TransactionPool;
}

namespace kagome::runtime {
  class TaggedTransactionQueue;
}

namespace kagome::service {
  class ExtrinsicSubmissionApi {
   public:
    /**
     * @constructor
     * @param api ttq instance shared ptr
     * @param pool transaction pool instance shared ptr
     * @param hasher hasher instance shared ptr
     */
    ExtrinsicSubmissionApi(
        std::shared_ptr<runtime::TaggedTransactionQueue> api,
        std::shared_ptr<transaction_pool::TransactionPool> pool,
        std::shared_ptr<hash::Hasher> hasher);

    /**
     * @brief validates and sends extrinsic to transaction pool
     * @param bytes encoded extrinsic
     * @return hash of successfully validated extrinsic
     * or error if state is invalid or unknown
     */
    outcome::result<common::Hash256> submit_extrinsic(
        const common::Buffer &bytes);

    /**
     * @return collection of pending extrinsics
     */
    outcome::result<std::vector<std::vector<uint8_t>>> pending_extrinsics();

    // TODO(yuraz): probably will be documented later (no task yet)
    outcome::result<std::vector<common::Hash256>> remove_extrinsic(
        const std::vector<boost::variant<std::vector<uint8_t>, common::Hash256>>
            &bytes_or_hash);

    // TODO(yuraz): probably will be documented later (no task yet)
    void watch_extrinsic(const primitives::Metadata &metadata,
                         const primitives::Subscriber &subscriber,
                         const common::Buffer &data);

    // TODO(yuraz): probably will be documented later (no task yet)
    outcome::result<bool> unwatch_extrinsic(
        const std::optional<primitives::Metadata> &metadata,
        const primitives::SubscriptionId &id);

   private:
    std::shared_ptr<runtime::TaggedTransactionQueue> api_;
    std::shared_ptr<transaction_pool::TransactionPool> pool_;
    std::shared_ptr<hash::Hasher> hasher_;
  };

}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_HPP
