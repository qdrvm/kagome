/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/parachain_host_types_serde.hpp"

namespace kagome::runtime {

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const ScheduledCore &val) {
    s << val.para_id << val.collator;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          ScheduledCore &val) {
    return s >> val.para_id >> val.collator;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateDescriptor &val) {
    s << val.para_id << val.relay_parent << val.collator
      << val.persisted_validation_data_hash << val.pov_hash << val.erasure_root
      << val.signature << val.para_head << val.validation_code_hash;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateDescriptor &val) {
    return s >> val.para_id >> val.relay_parent >> val.collator
           >> val.persisted_validation_data_hash >> val.pov_hash
           >> val.erasure_root >> val.signature >> val.para_head
           >> val.validation_code_hash;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const OccupiedCore &val) {
    s << val.next_up_on_available << val.occupied_since << val.time_out_at
      << val.next_up_on_time_out << val.availability << val.group_responsible
      << val.candidate_hash << val.candidate_descriptor;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          OccupiedCore &val) {
    return s >> val.next_up_on_available >> val.occupied_since
           >> val.time_out_at >> val.next_up_on_time_out >> val.availability
           >> val.group_responsible >> val.candidate_hash
           >> val.candidate_descriptor;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const PersistedValidationData &val) {
    s << val.parent_head << val.relay_parent_number
      << val.relay_parent_storage_root << val.max_pov_size;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          PersistedValidationData &val) {
    return s >> val.parent_head >> val.relay_parent_number
           >> val.relay_parent_storage_root >> val.max_pov_size;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const OutboundHrmpMessage &val) {
    s << val.recipient << val.data;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          OutboundHrmpMessage &val) {
    return s >> val.recipient >> val.data;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateCommitments &val) {
    s << val.upward_messages << val.horizontal_messages
      << val.new_validation_code << val.head_data
      << val.processed_downward_messages << val.hrmp_watermark;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateCommitments &val) {
    return s >> val.upward_messages >> val.horizontal_messages
           >> val.new_validation_code >> val.head_data
           >> val.processed_downward_messages >> val.hrmp_watermark;
  }

  ::scale::ScaleEncoderStream &operator<<(
      ::scale::ScaleEncoderStream &s, const CommittedCandidateReceipt &val) {
    s << val.descriptor << val.commitments;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CommittedCandidateReceipt &val) {
    return s >> val.descriptor >> val.commitments;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateReceipt &val) {
    s << val.descriptor << val.commitments_hash;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateReceipt &val) {
    return s >> val.descriptor >> val.commitments_hash;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const Candidate &val) {
    s << val.candidate_receipt << val.head_data << val.core_index;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          Candidate &val) {
    return s >> val.candidate_receipt >> val.head_data >> val.core_index;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateBacked &val) {
    s << static_cast<Candidate>(val) << val.group_index;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateBacked &val) {
    return s >> static_cast<Candidate &>(val) >> val.group_index;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateIncluded &val) {
    s << static_cast<Candidate>(val);
    s << val.group_index;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateIncluded &val) {
    return s >> static_cast<Candidate &>(val) >> val.group_index;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const SessionInfo &val) {
    s << val.active_validator_indices << val.random_seed << val.dispute_period
      << val.validators << val.discovery_keys << val.assignment_keys
      << val.validator_groups << val.n_cores << val.zeroth_delay_tranche_width
      << val.relay_vrf_modulo_samples << val.n_delay_tranches
      << val.no_show_slots << val.needed_approvals;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          SessionInfo &val) {
    return s >> val.active_validator_indices >> val.random_seed
           >> val.dispute_period >> val.validators >> val.discovery_keys
           >> val.assignment_keys >> val.validator_groups >> val.n_cores
           >> val.zeroth_delay_tranche_width >> val.relay_vrf_modulo_samples
           >> val.n_delay_tranches >> val.no_show_slots >> val.needed_approvals;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const InboundDownwardMessage &val) {
    s << val.sent_at << val.msg;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          InboundDownwardMessage &val) {
    return s >> val.sent_at >> val.msg;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const InboundHrmpMessage &val) {
    s << val.sent_at << val.data;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          InboundHrmpMessage &val) {
    return s >> val.sent_at >> val.data;
  }
}  // namespace kagome::runtime
