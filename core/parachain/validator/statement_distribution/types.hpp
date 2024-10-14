/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {

  struct StatementWithPVDSeconded {
    network::CommittedCandidateReceipt committed_receipt;
    runtime::PersistedValidationData pvd;
  };

  struct StatementWithPVDValid {
    parachain::CandidateHash candidate_hash;
  };

  using StatementWithPVD =
      boost::variant<StatementWithPVDSeconded, StatementWithPVDValid>;

  using SignedFullStatementWithPVD =
      parachain::IndexedAndSigned<StatementWithPVD>;

}  // namespace kagome::parachain
