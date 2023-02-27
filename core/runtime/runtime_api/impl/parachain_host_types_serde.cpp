/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/parachain_host_types_serde.hpp"

namespace kagome::runtime {

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

}  // namespace kagome::runtime
