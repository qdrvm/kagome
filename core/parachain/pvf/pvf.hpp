/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
    using Cb = std::function<void(outcome::result<Result>)>;

    virtual ~Pvf() = default;

    /// Execute pvf synchronously
    virtual void pvf(const CandidateReceipt &receipt,
                     const ParachainBlock &pov,
                     const runtime::PersistedValidationData &pvd,
                     Cb cb) const = 0;

    virtual void pvfValidate(const PersistedValidationData &data,
                             const ParachainBlock &pov,
                             const CandidateReceipt &receipt,
                             const ParachainRuntime &code,
                             Cb cb) const = 0;
  };
}  // namespace kagome::parachain
