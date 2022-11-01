/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_PVF_PVF_HPP
#define KAGOME_PARACHAIN_PVF_PVF_HPP

#include "network/types/collator_messages.hpp"
#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {
  /// Executes pvf (Parachain Validation Function, wasm)
  class Pvf {
   public:
    using CandidateReceipt = network::CandidateReceipt;
    using ParachainBlock = network::ParachainBlock;
    using CandidateCommitments = network::CandidateCommitments;
    using PersistedValidationData = runtime::PersistedValidationData;
    using Result = std::pair<CandidateCommitments, PersistedValidationData>;

    virtual ~Pvf() = default;

    /// Execute pvf synchronously
    virtual outcome::result<Result> pvfSync(
        const CandidateReceipt &receipt, const ParachainBlock &pov) const = 0;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_PVF_PVF_HPP
