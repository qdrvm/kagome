/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_API_EXTRINSIC_EXTRINSIC_API_HPP
#define KAGOME_CORE_API_EXTRINSIC_EXTRINSIC_API_HPP

#include "common/blob.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/extrinsic_api_primitives.hpp"

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
    virtual outcome::result<Hash256> submitExtrinsic(
        const Extrinsic &extrinsic) = 0;

    /**
     * @return collection of pending extrinsics
     */
    virtual outcome::result<std::vector<Extrinsic>> pendingExtrinsics() = 0;

    // TODO(yuraz): will be documented later (no task yet)
    virtual outcome::result<std::vector<Hash256>> removeExtrinsic(
        const std::vector<ExtrinsicKey> &keys) = 0;
  };
}  // namespace kagome::api

#endif  // KAGOME_CORE_API_EXTRINSIC_EXTRINSIC_API_HPP
