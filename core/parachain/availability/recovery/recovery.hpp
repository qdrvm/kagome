/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_RECOVERY_RECOVERY_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_RECOVERY_RECOVERY_HPP

#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {

  /// Used to recover PoV and validation data from remote validators inside
  /// validator group. This operation is important in Approval step to validate
  /// availability and correctness of the candidate.
  class Recovery {
   public:
    using AvailableData = runtime::AvailableData;
    using Cb =
        std::function<void(std::optional<outcome::result<AvailableData>>)>;
    using CandidateReceipt = network::CandidateReceipt;

    virtual ~Recovery() = default;

    virtual void recover(CandidateReceipt receipt,
                         SessionIndex session_index,
                         std::optional<GroupIndex> backing_group,
                         Cb cb) = 0;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_RECOVERY_RECOVERY_HPP
