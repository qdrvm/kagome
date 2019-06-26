/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_HPP

#include "common/blob.hpp"
#include "primitives/extrinsic_api_primitives.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::api {
  class ExtrinsicApi {
   protected:
    using Hash256 = common::Hash256;
    using Buffer = common::Buffer;
    using Extrinsic = primitives::Extrinsic;
    using Metadata = primitives::Metadata;
    using Subscriber = primitives::Subscriber;
    using SubscriptionId = primitives::SubscriptionId;
    using ExtrinsicKey = primitives::ExtrinsicKey;

   public:
    virtual ~ExtrinsicApi() = default;
    /**
     * @brief validates and sends extrinsic to transaction pool
     * @param bytes encoded extrinsic
     * @return hash of successfully validated extrinsic
     * or error if state is invalid or unknown
     */
    virtual outcome::result<Hash256> submit_extrinsic(
        const Extrinsic &extrinsic) = 0;

    /**
     * @return collection of pending extrinsics
     */
    virtual outcome::result<std::vector<Extrinsic>> pending_extrinsics() = 0;

    // TODO(yuraz): will be documented later (no task yet)
    virtual outcome::result<std::vector<Hash256>> remove_extrinsic(
        const std::vector<ExtrinsicKey> &bytes_or_hash) = 0;

    // TODO(yuraz): will be documented later (no task yet)
    virtual void watch_extrinsic(const Metadata &metadata,
                                 const Subscriber &subscriber,
                                 const Buffer &data) = 0;

    // TODO(yuraz): will be documented later (no task yet)
    virtual outcome::result<bool> unwatch_extrinsic(
        const std::optional<Metadata> &metadata, const SubscriptionId &id) = 0;
  };
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_HPP
