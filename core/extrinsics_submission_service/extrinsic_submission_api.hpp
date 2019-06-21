/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_HPP
#define KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_HPP

#include "primitives/auth_api.hpp"
#include "primitives/extrinsic.hpp"

namespace kagome::service {
  class ExtrinsicSubmissionApi {
   public:
    virtual ~ExtrinsicSubmissionApi() = default;
    /**
     * @brief validates and sends extrinsic to transaction pool
     * @param bytes encoded extrinsic
     * @return hash of successfully validated extrinsic
     * or error if state is invalid or unknown
     */
    virtual outcome::result<common::Hash256> submit_extrinsic(
        const primitives::Extrinsic &extrinsic) = 0;

    /**
     * @return collection of pending extrinsics
     */
    virtual outcome::result<std::vector<std::vector<uint8_t>>>
    pending_extrinsics() = 0;

    // TODO(yuraz): probably will be documented later (no task yet)
    virtual outcome::result<std::vector<common::Hash256>> remove_extrinsic(
        const std::vector<boost::variant<std::vector<uint8_t>, common::Hash256>>
            &bytes_or_hash) = 0;

    // TODO(yuraz): probably will be documented later (no task yet)
    virtual void watch_extrinsic(const primitives::Metadata &metadata,
                                 const primitives::Subscriber &subscriber,
                                 const common::Buffer &data) = 0;

    // TODO(yuraz): probably will be documented later (no task yet)
    virtual outcome::result<bool> unwatch_extrinsic(
        const std::optional<primitives::Metadata> &metadata,
        const primitives::SubscriptionId &id) = 0;
  };
}  // namespace kagome::service

#endif  // KAGOME_CORE_EXTRINSICS_SUBMISSION_SERVICE_EXTRINSIC_SUBMISSION_API_HPP
