/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_api/impl/parachain_host_types_serde.hpp"

namespace kagome::runtime {

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const ScheduledCore &val) {
    s << std::tie(val.para_id, val.collator);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          ScheduledCore &val) {
    std::tuple<ParachainId, std::optional<CollatorId>> fetched;
    s >> fetched;
    val = ScheduledCore{std::get<0>(fetched), std::get<1>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateDescriptor &val) {
    s << std::tie(val.para_id,
                  val.relay_parent,
                  val.collator,
                  val.persisted_validation_data_hash,
                  val.pov_hash,
                  val.erasure_root,
                  val.signature,
                  val.para_head,
                  val.validation_code_hash);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateDescriptor &val) {
    std::tuple<ParachainId,
               Hash,
               CollatorId,
               Hash,
               Hash,
               Hash,
               CollatorSignature,
               Hash,
               ValidationCodeHash>
        fetched;
    s >> fetched;
    val = CandidateDescriptor{std::get<0>(fetched),
                              std::get<1>(fetched),
                              std::get<2>(fetched),
                              std::get<3>(fetched),
                              std::get<4>(fetched),
                              std::get<5>(fetched),
                              std::get<6>(fetched),
                              std::get<7>(fetched),
                              std::get<8>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const OccupiedCore &val) {
    s << std::tie(val.next_up_on_available,
                  val.occupied_since,
                  val.time_out_at,
                  val.next_up_on_time_out,
                  val.availability,
                  val.group_responsible,
                  val.candidate_hash,
                  val.candidate_descriptor);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          OccupiedCore &val) {
    std::tuple<std::optional<ScheduledCore>,
               BlockNumber,
               BlockNumber,
               std::optional<ScheduledCore>,
               Bitvec,
               GroupIndex,
               CandidateHash,
               CandidateDescriptor>
        fetched;
    // s >> fetched;
    val = OccupiedCore{std::get<0>(fetched),
                       std::get<1>(fetched),
                       std::get<2>(fetched),
                       std::get<3>(fetched),
                       std::get<4>(fetched),
                       std::get<5>(fetched),
                       std::get<6>(fetched),
                       std::get<7>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const PersistedValidationData &val) {
    s << std::tie(val.parent_head,
                  val.relay_parent_number,
                  val.relay_parent_storage_root,
                  val.max_pov_size);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          PersistedValidationData &val) {
    std::tuple<HeadData, BlockNumber, Hash, uint32_t> fetched;
    s >> fetched;
    val = PersistedValidationData{std::get<0>(fetched),
                                  std::get<1>(fetched),
                                  std::get<2>(fetched),
                                  std::get<3>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const OutboundHrmpMessage &val) {
    s << std::tie(val.recipient, val.data);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          OutboundHrmpMessage &val) {
    std::tuple<ParachainId, Buffer> fetched;
    s >> fetched;
    val = OutboundHrmpMessage{std::get<0>(fetched), std::get<1>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateCommitments &val) {
    s << std::tie(val.upward_messages,
                  val.horizontal_messages,
                  val.new_validation_code,
                  val.head_data,
                  val.processed_downward_messages,
                  val.hrmp_watermark);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateCommitments &val) {
    std::tuple<std::vector<UpwardMessage>,
               std::vector<OutboundHrmpMessage>,
               std::optional<ValidationCode>,
               HeadData,
               uint32_t,
               BlockNumber>
        fetched;
    s >> fetched;
    val = CandidateCommitments{std::get<0>(fetched),
                               std::get<1>(fetched),
                               std::get<2>(fetched),
                               std::get<3>(fetched),
                               std::get<4>(fetched),
                               std::get<5>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(
      ::scale::ScaleEncoderStream &s, const CommittedCandidateReceipt &val) {
    s << std::tie(val.descriptor, val.commitments);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CommittedCandidateReceipt &val) {
    std::tuple<CandidateDescriptor, CandidateCommitments> fetched;
    s >> fetched;
    val = CommittedCandidateReceipt{std::get<0>(fetched), std::get<1>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateReceipt &val) {
    s << std::tie(val.descriptor, val.commitments_hash);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateReceipt &val) {
    std::tuple<CandidateDescriptor, Hash> fetched;
    s >> fetched;
    val = CandidateReceipt{std::get<0>(fetched), std::get<1>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const Candidate &val) {
    s << std::tie(val.candidate_receipt, val.head_data, val.core_index);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          Candidate &val) {
    std::tuple<CandidateReceipt, HeadData, CoreIndex> fetched;
    s >> fetched;
    val = Candidate{
        std::get<0>(fetched), std::get<1>(fetched), std::get<2>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateBacked &val) {
    s << static_cast<Candidate>(val);
    s << val.group_index;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateBacked &val) {
    Candidate fetched;
    GroupIndex group_index;
    s >> fetched;
    s >> group_index;
    val = CandidateBacked{fetched.candidate_receipt,
                          fetched.head_data,
                          fetched.core_index,
                          group_index};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateIncluded &val) {
    s << static_cast<Candidate>(val);
    s << val.group_index;
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateIncluded &val) {
    Candidate fetched;
    GroupIndex group_index;
    s >> fetched;
    s >> group_index;
    val = CandidateIncluded{fetched.candidate_receipt,
                            fetched.head_data,
                            fetched.core_index,
                            group_index};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const SessionInfo &val) {
    s << std::tie(val.active_validator_indices,
                  val.random_seed,
                  val.dispute_period,
                  val.validators,
                  val.discovery_keys,
                  val.assignment_keys,
                  val.validator_groups,
                  val.n_cores,
                  val.zeroth_delay_tranche_width,
                  val.relay_vrf_modulo_samples,
                  val.n_delay_tranches,
                  val.no_show_slots,
                  val.needed_approvals);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          SessionInfo &val) {
    std::tuple<std::vector<ValidatorIndex>,
               common::Blob<32>,
               SessionIndex,
               std::vector<ValidatorId>,
               std::vector<AuthorityDiscoveryId>,
               std::vector<AssignmentId>,
               std::vector<std::vector<ValidatorIndex>>,
               uint32_t,
               uint32_t,
               uint32_t,
               uint32_t,
               uint32_t,
               uint32_t>
        fetched;
    s >> fetched;
    val = SessionInfo{std::get<0>(fetched),
                      std::get<1>(fetched),
                      std::get<2>(fetched),
                      std::get<3>(fetched),
                      std::get<4>(fetched),
                      std::get<5>(fetched),
                      std::get<6>(fetched),
                      std::get<7>(fetched),
                      std::get<8>(fetched),
                      std::get<9>(fetched),
                      std::get<10>(fetched),
                      std::get<11>(fetched),
                      std::get<12>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const InboundDownwardMessage &val) {
    s << std::tie(val.sent_at, val.msg);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          InboundDownwardMessage &val) {
    std::tuple<BlockNumber, DownwardMessage> fetched;
    s >> fetched;
    val = InboundDownwardMessage{std::get<0>(fetched), std::get<1>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const InboundHrmpMessage &val) {
    s << std::tie(val.sent_at, val.data);
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          InboundHrmpMessage &val) {
    std::tuple<BlockNumber, Buffer> fetched;
    s >> fetched;
    val = InboundHrmpMessage{std::get<0>(fetched), std::get<1>(fetched)};
    return s;
  }

  ::scale::ScaleEncoderStream &operator<<(
      ::scale::ScaleEncoderStream &s,
      const std::map<unsigned int,
                     std::vector<kagome::runtime::InboundHrmpMessage>>
          &hrmp_map) {
    return s;
  }

  ::scale::ScaleDecoderStream &operator>>(
      ::scale::ScaleDecoderStream &s,
      std::map<unsigned int, std::vector<kagome::runtime::InboundHrmpMessage>>
          &hrmp_map) {
    return s;
  }
}  // namespace kagome::runtime
