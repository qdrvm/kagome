/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/type_hasher.hpp"
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
    using HashedCandidateReceipt = crypto::
        Hashed<network::CandidateReceipt, 32, crypto::Blake2b_StreamHasher<32>>;

    virtual ~Recovery() = default;

    virtual void remove(const CandidateHash &candidate) = 0;
    virtual void recover(const HashedCandidateReceipt &hashed_receipt,
                         SessionIndex session_index,
                         std::optional<GroupIndex> backing_group,
                         Cb cb) = 0;
  };
}  // namespace kagome::parachain
