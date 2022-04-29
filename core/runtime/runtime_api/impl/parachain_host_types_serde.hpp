/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_SERDE_HPP
#define KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_SERDE_HPP

#include "runtime/runtime_api/parachain_host_types.hpp"
#include "scale/scale.hpp"

namespace kagome::runtime {

  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const ScheduledCore &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          ScheduledCore &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateDescriptor &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateDescriptor &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const OccupiedCore &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          OccupiedCore &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const PersistedValidationData &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          PersistedValidationData &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const OutboundHrmpMessage &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          OutboundHrmpMessage &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateCommitments &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateCommitments &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CommittedCandidateReceipt &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CommittedCandidateReceipt &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateReceipt &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateReceipt &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const Candidate &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          Candidate &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateBacked &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateBacked &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const CandidateIncluded &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          CandidateIncluded &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const SessionInfo &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          SessionInfo &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const InboundDownwardMessage &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          InboundDownwardMessage &val);
  ::scale::ScaleEncoderStream &operator<<(::scale::ScaleEncoderStream &s,
                                          const InboundHrmpMessage &val);
  ::scale::ScaleDecoderStream &operator>>(::scale::ScaleDecoderStream &s,
                                          InboundHrmpMessage &val);
  ::scale::ScaleEncoderStream &operator<<(
      ::scale::ScaleEncoderStream &s,
      const std::map<unsigned int,
                     std::vector<kagome::runtime::InboundHrmpMessage>>
          &hrmp_map);
  ::scale::ScaleDecoderStream &operator>>(
      ::scale::ScaleDecoderStream &s,
      std::map<unsigned int, std::vector<kagome::runtime::InboundHrmpMessage>>
          &hrmp_map);
}  // namespace kagome::runtime

#endif  // KAGOME_CORE_RUNTIME_IMPL_PARACHAIN_HOST_SERDE_HPP
