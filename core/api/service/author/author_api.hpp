/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_API_EXTRINSIC_EXTRINSIC_API_HPP
#define KAGOME_CORE_API_EXTRINSIC_EXTRINSIC_API_HPP

#include "api/service/api_service.hpp"
#include "common/blob.hpp"
#include "primitives/author_api_primitives.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api {
  class AuthorApi {
   protected:
    using Hash256 = common::Hash256;
    using Buffer = common::Buffer;
    using Extrinsic = primitives::Extrinsic;
    using Metadata = primitives::Metadata;
    using SubscriptionId = primitives::SubscriptionId;
    using ExtrinsicKey = primitives::ExtrinsicKey;

   public:
    virtual ~AuthorApi() = default;

    virtual void setApiService(
        std::shared_ptr<api::ApiService> const &api_service) = 0;

    /**
     * @brief validates and sends extrinsic to transaction pool
     * @param bytes encoded extrinsic
     * @return hash of successfully validated extrinsic
     * or error if state is invalid or unknown
     */
    virtual outcome::result<Hash256> submitExtrinsic(
        const Extrinsic &extrinsic) = 0;

    /**
     * @return collection of pending extrinsics
     */
    virtual outcome::result<std::vector<Extrinsic>> pendingExtrinsics() = 0;

    // TODO(yuraz): will be documented later (no task yet)
    virtual outcome::result<std::vector<Hash256>> removeExtrinsic(
        const std::vector<ExtrinsicKey> &keys) = 0;

    /**
     * Submit an extrinsic and watch.
     */
    virtual outcome::result<SubscriptionId> submitAndWatchExtrinsic(
        Extrinsic extrinsic) = 0;

    /**
     * Unsubscribe from extrinsic watching.
     * @see AuthorApi::submitAndWatchExtrinsic
     * @return true if the subscriber was unsubscribed, false if there was no
     * subscriber.
     */
    virtual outcome::result<bool> unwatchExtrinsic(SubscriptionId sub_id) = 0;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_EXTRINSIC_EXTRINSIC_API_HPP
