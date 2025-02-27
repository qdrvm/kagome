/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/prospective_parachains/fragment.hpp"

#include "parachain/ump_signal.hpp"
#include "utils/stringify.hpp"

#define COMPONENT_NAME "Fragment"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::parachain::fragment, Fragment::Error, e) {
  using E = decltype(e);
  switch (e) {
    case E::HRMP_MESSAGE_DESCENDING_OR_DUPLICATE:
      return COMPONENT_NAME
          ": Horizontal message has descending order or duplicate";
    case E::PERSISTED_VALIDATION_DATA_MISMATCH:
      return COMPONENT_NAME ": persisted validation data mismatch";
    case E::VALIDATION_CODE_MISMATCH:
      return COMPONENT_NAME ": validation code mismatch by hash";
    case E::RELAY_PARENT_TOO_OLD:
      return COMPONENT_NAME ": relay parent too old";
    case E::CODE_UPGRADE_RESTRICTED:
      return COMPONENT_NAME ": code upgrade restricted";
    case E::CODE_SIZE_TOO_LARGE:
      return COMPONENT_NAME ": code size too large";
    case E::DMP_ADVANCEMENT_RULE:
      return COMPONENT_NAME ": dmp advancement rule";
    case E::HRMP_MESSAGES_PER_CANDIDATE_OVERFLOW:
      return COMPONENT_NAME ": hrmp messages per candidate overflow";
    case E::UMP_MESSAGES_PER_CANDIDATE_OVERFLOW:
      return COMPONENT_NAME ": ump messages per candidate overflow";
  }
  return COMPONENT_NAME ": unknown error";
}

namespace kagome::parachain::fragment {

  outcome::result<void> validate_against_constraints(
      const Constraints &constraints,
      const RelayChainBlockInfo &relay_parent,
      const CandidateCommitments &commitments,
      const PersistedValidationData &persisted_validation_data,
      const ValidationCodeHash &validation_code_hash,
      const ConstraintModifications &modifications) {
    runtime::PersistedValidationData expected_pvd{
        .parent_head = constraints.required_parent,
        .relay_parent_number = relay_parent.number,
        .relay_parent_storage_root = relay_parent.storage_root,
        .max_pov_size = uint32_t(constraints.max_pov_size),
    };

    if (expected_pvd != persisted_validation_data) {
      return Fragment::Error::PERSISTED_VALIDATION_DATA_MISMATCH;
    }

    if (constraints.validation_code_hash != validation_code_hash) {
      return Fragment::Error::VALIDATION_CODE_MISMATCH;
    }

    if (relay_parent.number < constraints.min_relay_parent_number) {
      return Fragment::Error::RELAY_PARENT_TOO_OLD;
    }

    if (commitments.opt_para_runtime && constraints.upgrade_restriction
        && *constraints.upgrade_restriction == UpgradeRestriction::Present) {
      return Fragment::Error::CODE_UPGRADE_RESTRICTED;
    }

    const size_t announced_code_size = commitments.opt_para_runtime
                                         ? commitments.opt_para_runtime->size()
                                         : 0ull;
    if (announced_code_size > constraints.max_code_size) {
      return Fragment::Error::CODE_SIZE_TOO_LARGE;
    }

    if (modifications.dmp_messages_processed == 0
        && !constraints.dmp_remaining_messages.empty()
        && constraints.dmp_remaining_messages[0] <= relay_parent.number) {
      return Fragment::Error::DMP_ADVANCEMENT_RULE;
    }

    if (commitments.outbound_hor_msgs.size()
        > constraints.max_hrmp_num_per_candidate) {
      return Fragment::Error::HRMP_MESSAGES_PER_CANDIDATE_OVERFLOW;
    }

    if (commitments.upward_msgs.size()
        > constraints.max_ump_num_per_candidate) {
      return Fragment::Error::UMP_MESSAGES_PER_CANDIDATE_OVERFLOW;
    }

    return constraints.check_modifications(modifications);
  }

  const RelayChainBlockInfo &Fragment::get_relay_parent() const {
    return relay_parent;
  }

  outcome::result<Fragment> Fragment::create(
      const RelayChainBlockInfo &relay_parent,
      const Constraints &operating_constraints,
      const std::shared_ptr<const ProspectiveCandidate> &candidate) {
    OUTCOME_TRY(
        modifications,
        check_against_constraints(relay_parent,
                                  operating_constraints,
                                  candidate->commitments,
                                  candidate->validation_code_hash,
                                  candidate->persisted_validation_data));

    return Fragment{
        .relay_parent = relay_parent,
        .operating_constraints = operating_constraints,
        .candidate = candidate,
        .modifications = modifications,
    };
  }

  outcome::result<ConstraintModifications> Fragment::check_against_constraints(
      const RelayChainBlockInfo &relay_parent,
      const Constraints &operating_constraints,
      const CandidateCommitments &commitments,
      const ValidationCodeHash &validation_code_hash,
      const PersistedValidationData &persisted_validation_data) {
    HashMap<ParachainId, OutboundHrmpChannelModification> outbound_hrmp;
    {
      Option<ParachainId> last_recipient;
      for (const auto &message : commitments.outbound_hor_msgs) {
        if (last_recipient && *last_recipient >= message.recipient) {
          return Error::HRMP_MESSAGE_DESCENDING_OR_DUPLICATE;
        }
        last_recipient = message.recipient;
        OutboundHrmpChannelModification &record =
            outbound_hrmp[message.recipient];

        record.bytes_submitted += message.data.size();
        record.messages_submitted += 1;
      }
    }

    uint32_t ump_sent_bytes = 0ull;
    for (const auto &m : skipUmpSignals(commitments.upward_msgs)) {
      ump_sent_bytes += uint32_t(m.size());
    }

    ConstraintModifications modifications{
        .required_parent = commitments.para_head,
        .hrmp_watermark = ((commitments.watermark == relay_parent.number)
                               ? HrmpWatermarkUpdate{HrmpWatermarkUpdateHead{
                                     .v = commitments.watermark}}
                               : HrmpWatermarkUpdate{HrmpWatermarkUpdateTrunk{
                                     .v = commitments.watermark}}),
        .outbound_hrmp = outbound_hrmp,
        .ump_messages_sent = uint32_t(commitments.upward_msgs.size()),
        .ump_bytes_sent = ump_sent_bytes,
        .dmp_messages_processed = commitments.downward_msgs_count,
        .code_upgrade_applied =
            operating_constraints.future_validation_code
            && relay_parent.number
                   >= operating_constraints.future_validation_code->first,
    };

    OUTCOME_TRY(validate_against_constraints(operating_constraints,
                                             relay_parent,
                                             commitments,
                                             persisted_validation_data,
                                             validation_code_hash,
                                             modifications));
    return modifications;
  }

  const ConstraintModifications &Fragment::constraint_modifications() const {
    return modifications;
  }

  const Constraints &Fragment::get_operating_constraints() const {
    return operating_constraints;
  }

  const ProspectiveCandidate &Fragment::get_candidate() const {
    BOOST_ASSERT_MSG(candidate != nullptr, "Candidate is null");
    return *candidate;
  }

  std::shared_ptr<const ProspectiveCandidate> Fragment::get_candidate_clone()
      const {
    return candidate;
  }

}  // namespace kagome::parachain::fragment
