/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_HPP
#define KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_HPP

#include "common/lru_cache.hpp"
#include "primitives/block_id.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome::runtime {

  class Executor;

  class ParachainHostImpl final : public ParachainHost {
   public:
    explicit ParachainHostImpl(std::shared_ptr<Executor> executor);

    outcome::result<DutyRoster> duty_roster(
        const primitives::BlockHash &block) override;

    outcome::result<std::vector<ParachainId>> active_parachains(
        const primitives::BlockHash &block) override;

    outcome::result<std::optional<Buffer>> parachain_head(
        const primitives::BlockHash &block, ParachainId id) override;

    outcome::result<std::optional<Buffer>> parachain_code(
        const primitives::BlockHash &block, ParachainId id) override;

    outcome::result<std::vector<ValidatorId>> validators(
        const primitives::BlockHash &block) override;

    outcome::result<ValidatorGroupsAndDescriptor> validator_groups(
        const primitives::BlockHash &block) override;

    outcome::result<std::vector<CoreState>> availability_cores(
        const primitives::BlockHash &block) override;

    outcome::result<std::optional<PersistedValidationData>>
    persisted_validation_data(const primitives::BlockHash &block,
                              ParachainId id,
                              OccupiedCoreAssumption assumption) override;

    outcome::result<bool> check_validation_outputs(
        const primitives::BlockHash &block,
        ParachainId id,
        CandidateCommitments outputs) override;

    outcome::result<SessionIndex> session_index_for_child(
        const primitives::BlockHash &block) override;

    outcome::result<std::optional<ValidationCode>> validation_code(
        const primitives::BlockHash &block,
        ParachainId id,
        OccupiedCoreAssumption assumption) override;

    outcome::result<std::optional<ValidationCode>> validation_code_by_hash(
        const primitives::BlockHash &block, ValidationCodeHash hash) override;

    outcome::result<std::optional<CommittedCandidateReceipt>>
    candidate_pending_availability(const primitives::BlockHash &block,
                                   ParachainId id) override;

    outcome::result<std::vector<CandidateEvent>> candidate_events(
        const primitives::BlockHash &block) override;

    outcome::result<std::optional<SessionInfo>> session_info(
        const primitives::BlockHash &block, SessionIndex index) override;

    outcome::result<std::vector<InboundDownwardMessage>> dmq_contents(
        const primitives::BlockHash &block, ParachainId id) override;

    outcome::result<std::map<ParachainId, std::vector<InboundHrmpMessage>>>
    inbound_hrmp_channels_contents(const primitives::BlockHash &block,
                                   ParachainId id) override;

   private:
    std::shared_ptr<Executor> executor_;

    template <typename T, typename... Ts>
    using C = LruCache<std::tuple<primitives::BlockHash, Ts...>, T, true>;

    C<std::vector<ParachainId>> active_parachains_{10};
    C<std::optional<Buffer>> parachain_head_{10};
    C<std::optional<Buffer>, ParachainId> parachain_code_{10};
    C<std::vector<ValidatorId>> validators_{10};
    C<ValidatorGroupsAndDescriptor> validator_groups_{10};
    C<std::vector<CoreState>> availability_cores_{10};
    C<SessionIndex> session_index_for_child_{10};
    C<std::optional<Buffer>, ValidationCodeHash> validation_code_by_hash_{10};
    C<std::optional<CommittedCandidateReceipt>, ParachainId>
        candidate_pending_availability_{10};
    C<std::vector<CandidateEvent>> candidate_events_{10};
    C<std::optional<SessionInfo>, SessionIndex> session_info_{10};
    C<std::vector<InboundDownwardMessage>, ParachainId> dmq_contents_{10};
    C<std::map<ParachainId, std::vector<InboundHrmpMessage>>, ParachainId>
        inbound_hrmp_channels_contents_{10};
  };

}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP
