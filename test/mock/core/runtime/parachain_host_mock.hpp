/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/parachain_host.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class ParachainHostMock final : public ParachainHost {
   public:
    MOCK_METHOD(outcome::result<DutyRoster>,
                duty_roster,
                (const primitives::BlockHash &block),
                (override));
    MOCK_METHOD(outcome::result<std::vector<ParachainId>>,
                active_parachains,
                (const primitives::BlockHash &block),
                (override));
    MOCK_METHOD(outcome::result<std::optional<Buffer>>,
                parachain_head,
                (const primitives::BlockHash &block, ParachainId id),
                (override));
    MOCK_METHOD(outcome::result<std::optional<kagome::common::Buffer>>,
                parachain_code,
                (const primitives::BlockHash &block, ParachainId id),
                (override));
    MOCK_METHOD(outcome::result<std::vector<ValidatorId>>,
                validators,
                (const primitives::BlockHash &block),
                (override));
    MOCK_METHOD(outcome::result<ValidatorGroupsAndDescriptor>,
                validator_groups,
                (const primitives::BlockHash &block),
                (override));
    MOCK_METHOD(outcome::result<std::vector<CoreState>>,
                availability_cores,
                (const primitives::BlockHash &block),
                (override));
    MOCK_METHOD(outcome::result<std::optional<PersistedValidationData>>,
                persisted_validation_data,
                (const primitives::BlockHash &block,
                 ParachainId id,
                 OccupiedCoreAssumption assumption),
                (override));
    MOCK_METHOD(outcome::result<bool>,
                check_validation_outputs,
                (const primitives::BlockHash &block,
                 ParachainId id,
                 CandidateCommitments outputs),
                (override));
    MOCK_METHOD(outcome::result<SessionIndex>,
                session_index_for_child,
                (const primitives::BlockHash &block),
                (override));
    MOCK_METHOD(outcome::result<std::optional<ValidationCode>>,
                validation_code,
                (const primitives::BlockHash &block,
                 ParachainId id,
                 OccupiedCoreAssumption assumption),
                (override));
    MOCK_METHOD(outcome::result<std::optional<ValidationCode>>,
                validation_code_by_hash,
                (const primitives::BlockHash &block, ValidationCodeHash hash),
                (override));
    MOCK_METHOD(outcome::result<std::optional<CommittedCandidateReceipt>>,
                candidate_pending_availability,
                (const primitives::BlockHash &block, ParachainId id),
                (override));
    MOCK_METHOD(outcome::result<std::vector<CandidateEvent>>,
                candidate_events,
                (const primitives::BlockHash &block),
                (override));
    MOCK_METHOD(outcome::result<std::optional<SessionInfo>>,
                session_info,
                (const primitives::BlockHash &block, SessionIndex index),
                (override));
    MOCK_METHOD(outcome::result<std::vector<InboundDownwardMessage>>,
                dmq_contents,
                (const primitives::BlockHash &block, ParachainId id),
                (override));
    MOCK_METHOD(
        (outcome::result<std::map<ParachainId, std::vector<InboundHrmpMessage>>>),
        inbound_hrmp_channels_contents,
        (const primitives::BlockHash &block, ParachainId id),
        (override));
    MOCK_METHOD(outcome::result<std::optional<std::vector<ExecutorParam>>>,
                session_executor_params,
                (const primitives::BlockHash &block, SessionIndex idx),
                (override));
  };

}  // namespace kagome::runtime
