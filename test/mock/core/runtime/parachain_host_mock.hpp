/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RUNTIME_PARQACHAINHOSTMOCK
#define KAGOME_RUNTIME_PARQACHAINHOSTMOCK

#include "runtime/runtime_api/parachain_host.hpp"

#include <gmock/gmock.h>

namespace kagome::runtime {

  class ParachainHostMock : public ParachainHost {
   public:
    MOCK_METHOD(outcome::result<DutyRoster>,
                duty_roster,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<std::vector<ParachainId>>,
                active_parachains,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<std::optional<Buffer>>,
                parachain_head,
                (const primitives::BlockHash &, ParachainId),
                (override));

    MOCK_METHOD(outcome::result<std::optional<kagome::common::Buffer>>,
                parachain_code,
                (const primitives::BlockHash &, ParachainId),
                (override));

    MOCK_METHOD(outcome::result<std::vector<ValidatorId>>,
                validators,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<ValidatorGroupsAndDescriptor>,
                validator_groups,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<std::vector<CoreState>>,
                availability_cores,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<std::optional<PersistedValidationData>>,
                persisted_validation_data,
                (const primitives::BlockHash &,
                 ParachainId,
                 OccupiedCoreAssumption),
                (override));

    MOCK_METHOD(outcome::result<bool>,
                check_validation_outputs,
                (const primitives::BlockHash &,
                 ParachainId,
                 CandidateCommitments),
                (override));

    MOCK_METHOD(outcome::result<SessionIndex>,
                session_index_for_child,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<std::optional<ValidationCode>>,
                validation_code,
                (const primitives::BlockHash &,
                 ParachainId,
                 OccupiedCoreAssumption),
                (override));

    MOCK_METHOD(outcome::result<std::optional<ValidationCode>>,
                validation_code_by_hash,
                (const primitives::BlockHash &, ValidationCodeHash),
                (override));

    MOCK_METHOD(outcome::result<std::optional<CommittedCandidateReceipt>>,
                candidate_pending_availability,
                (const primitives::BlockHash &, ParachainId),
                (override));

    MOCK_METHOD(outcome::result<std::vector<CandidateEvent>>,
                candidate_events,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<std::optional<SessionInfo>>,
                session_info,
                (const primitives::BlockHash &, SessionIndex),
                (override));

    MOCK_METHOD(outcome::result<std::vector<InboundDownwardMessage>>,
                dmq_contents,
                (const primitives::BlockHash &, ParachainId),
                (override));

    using ResType_inbound_hrmp_channels_contents =
        outcome::result<std::map<ParachainId, std::vector<InboundHrmpMessage>>>;
    MOCK_METHOD(ResType_inbound_hrmp_channels_contents,
                inbound_hrmp_channels_contents,
                (const primitives::BlockHash &, ParachainId),
                (override));

    MOCK_METHOD(outcome::result<std::optional<dispute::ScrapedOnChainVotes>>,
                on_chain_votes,
                (const primitives::BlockHash &),
                (override));

    using ResType_disputes =
        outcome::result<std::vector<std::tuple<dispute::SessionIndex,
                                               dispute::CandidateHash,
                                               dispute::DisputeState>>>;
    MOCK_METHOD(ResType_disputes,
                disputes,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<std::vector<ValidationCodeHash>>,
                pvfs_require_precheck,
                (const primitives::BlockHash &),
                (override));

    MOCK_METHOD(outcome::result<void>,
                submit_pvf_check_statement,
                (const primitives::BlockHash &,
                 const parachain::PvfCheckStatement &,
                 const parachain::Signature &),
                (override));
  };

}  // namespace kagome::runtime

#endif  // KAGOME_RUNTIME_PARQACHAINHOSTMOCK
