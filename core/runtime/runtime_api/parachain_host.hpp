/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP
#define KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP

#include "common/blob.hpp"
#include "common/unused.hpp"
#include "dispute_coordinator/provisioner/impl/prioritized_selection.hpp"
#include "dispute_coordinator/types.hpp"
#include "parachain/types.hpp"
#include "primitives/block_id.hpp"
#include "primitives/common.hpp"
#include "primitives/parachain_host.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::runtime {

  class ParachainHost {
   protected:
   public:
    virtual ~ParachainHost() = default;

    /**
     * @brief Calls the ParachainHost_active_parachains function from wasm code
     * @return vector of ParachainId items or error if fails
     */
    virtual outcome::result<std::vector<ParachainId>> active_parachains(
        const primitives::BlockHash &block) = 0;

    /**
     * @brief Calls the ParachainHost_parachain_head function from wasm code
     * @param id parachain id
     * @return parachain head or error if fails
     */
    virtual outcome::result<std::optional<Buffer>> parachain_head(
        const primitives::BlockHash &block, ParachainId id) = 0;

    /**
     * @brief Calls the ParachainHost_parachain_code function from wasm code
     * @param id parachain id
     * @return parachain code or error if fails
     */
    virtual outcome::result<std::optional<kagome::common::Buffer>>
    parachain_code(const primitives::BlockHash &block, ParachainId id) = 0;

    /**
     * @brief reports validators list for given block_id
     * @return validators list
     */
    virtual outcome::result<std::vector<ValidatorId>> validators(
        const primitives::BlockHash &block) = 0;

    /**
     * @brief Returns the validator groups and rotation info localized based on
     * the hypothetical child of a block whose state  this is invoked on. Note
     * that `now` in the `GroupRotationInfo`should be the successor of the
     * number of the block.
     * @return vector of validator groups
     */
    virtual outcome::result<ValidatorGroupsAndDescriptor> validator_groups(
        const primitives::BlockHash &block) = 0;

    /**
     * @brief Yields information on all availability cores as relevant to the
     * child block. Cores are either free or occupied. Free cores can have paras
     * assigned to them.
     * @return vector of core states
     */
    virtual outcome::result<std::vector<CoreState>> availability_cores(
        const primitives::BlockHash &block) = 0;

    /**
     * @brief Yields the persisted validation data for the given `ParaId` along
     * with an assumption that should be used if the para currently occupies a
     * core.
     * @param id parachain id
     * @param assumption occupied core assumption
     * @return Returns nullopt if either the para is not registered or the
     * assumption is `Freed` (not `Included`) and the para already occupies a
     * core.
     */
    virtual outcome::result<std::optional<PersistedValidationData>>
    persisted_validation_data(const primitives::BlockHash &block,
                              ParachainId id,
                              OccupiedCoreAssumption assumption) = 0;

    /**
     * @brief Checks if the given validation outputs pass the acceptance
     * criteria.
     * @param id parachain id
     * @param outputs candidate commitments
     * @return validity (bool)
     */
    virtual outcome::result<bool> check_validation_outputs(
        const primitives::BlockHash &block,
        ParachainId id,
        CandidateCommitments outputs) = 0;

    /**
     * @brief Returns the session index expected at a child of the block. This
     * can be used to instantiate a `SigningContext`.
     * @return session index
     */
    virtual outcome::result<SessionIndex> session_index_for_child(
        const primitives::BlockHash &block) = 0;

    /**
     * @brief Fetch the validation code used by a para, making the given
     * `OccupiedCoreAssumption`.
     * @param id parachain id
     * @param assumption occupied core assumption
     * @return nullopt if either the para is not registered or the assumption is
     * `Freed` (TimedOut or Unused) and the para already occupies a core.
     */
    virtual outcome::result<std::optional<ValidationCode>> validation_code(
        const primitives::BlockHash &block,
        ParachainId id,
        OccupiedCoreAssumption assumption) = 0;

    /**
     * @brief Get the validation code (runtime) from its hash.
     * @param hash validation code hash
     * @return validation code, if found
     */
    virtual outcome::result<std::optional<ValidationCode>>
    validation_code_by_hash(const primitives::BlockHash &block,
                            ValidationCodeHash hash) = 0;

    /**
     * @brief Get the receipt of a candidate pending availability.
     * @param id parachain id
     * @return This returns CommittedCandidateReceipt for any paras assigned to
     * occupied cores in `availability_cores` and nullopt otherwise.
     */
    virtual outcome::result<std::optional<CommittedCandidateReceipt>>
    candidate_pending_availability(const primitives::BlockHash &block,
                                   ParachainId id) = 0;

    ///
    /**
     * @brief Get a vector of events concerning candidates that occurred within
     * a block.
     * @return vector of events
     */
    virtual outcome::result<std::vector<CandidateEvent>> candidate_events(
        const primitives::BlockHash &block) = 0;

    /**
     * @brief  Get the session info for the given session, if stored.
     * @note This function is only available since parachain host version 2.
     * @param index session index
     * @return SessionInfo, optional
     */
    virtual outcome::result<std::optional<SessionInfo>> session_info(
        const primitives::BlockHash &block, SessionIndex index) = 0;

    /**
     * @brief Get all the pending inbound messages in the downward message queue
     * for a para.
     * @param id parachain id
     * @return vector of messages
     */
    virtual outcome::result<std::vector<InboundDownwardMessage>> dmq_contents(
        const primitives::BlockHash &block, ParachainId id) = 0;

    /**
     * @brief Get the contents of all channels addressed to the given recipient.
     * Channels that have no messages in them are also included.
     * @return Map of vectors of an inbound HRMP messages (Horizontal
     * Relay-routed Message Passing) per parachain
     */
    virtual outcome::result<
        std::map<ParachainId, std::vector<InboundHrmpMessage>>>
    inbound_hrmp_channels_contents(const primitives::BlockHash &block,
                                   ParachainId id) = 0;

    virtual outcome::result<std::optional<std::vector<ExecutorParam>>>
    session_executor_params(const primitives::BlockHash &block,
                            SessionIndex idx) = 0;

    /// Get all disputes in relation to a relay parent.
    virtual outcome::result<std::optional<dispute::ScrapedOnChainVotes>>
    on_chain_votes(const primitives::BlockHash &block) = 0;

    /// Returns all on-chain disputes at given block number. Available in `v3`.
    virtual outcome::result<std::vector<std::tuple<dispute::SessionIndex,
                                                   dispute::CandidateHash,
                                                   dispute::DisputeState>>>
    disputes(const primitives::BlockHash &block) = 0;

    /**
     * @return list of pvf requiring precheck
     */
    virtual outcome::result<std::vector<ValidationCodeHash>>
    pvfs_require_precheck(const primitives::BlockHash &block) = 0;

    /**
     * @return submit pvf check statement
     */
    virtual outcome::result<void> submit_pvf_check_statement(
        const primitives::BlockHash &block,
        const parachain::PvfCheckStatement &statement,
        const parachain::Signature &signature) = 0;

    /**
     * @return the state of parachain backing for a given para.
     */
    virtual outcome::result<std::optional<parachain::fragment::BackingState>>
    staging_para_backing_state(const primitives::BlockHash &block,
                               ParachainId id) = 0;

    /**
     * @return candidate's acceptance limitations for asynchronous backing for a
     * relay parent.
     */
    virtual outcome::result<parachain::fragment::AsyncBackingParams>
    staging_async_backing_params(const primitives::BlockHash &block) = 0;
  };

}  // namespace kagome::runtime
#endif  // KAGOME_CORE_RUNTIME_PARACHAIN_HOST_HPP
