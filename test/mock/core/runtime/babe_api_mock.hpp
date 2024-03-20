/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/babe_api.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class BabeApiMock : public BabeApi {
   public:
    MOCK_METHOD(outcome::result<consensus::babe::BabeConfiguration>,
                configuration,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<consensus::babe::Epoch>,
                next_epoch,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<
                    std::optional<consensus::babe::OpaqueKeyOwnershipProof>>,
                generate_key_ownership_proof,
                (const primitives::BlockHash &,
                 consensus::SlotNumber,
                 consensus::babe::AuthorityId),
                (override));

    MOCK_METHOD(outcome::result<void>,
                submit_report_equivocation_unsigned_extrinsic,
                (const primitives::BlockHash &,
                 consensus::babe::EquivocationProof,
                 consensus::babe::OpaqueKeyOwnershipProof),
                (override));

    MOCK_METHOD(outcome::result<std::vector<consensus::AuthorityIndex>>,
                disabled_validators,
                (const primitives::BlockHash &block),
                (override));
  };

}  // namespace kagome::runtime
