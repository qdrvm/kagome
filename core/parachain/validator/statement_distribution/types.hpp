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

  inline primitives::BlockHash candidateHashFrom(
      const StatementWithPVD &statement,
      const std::shared_ptr<crypto::Hasher> &hasher) {
    return visit_in_place(
        statement,
        [&](const StatementWithPVDSeconded &val) {
          return hasher->blake2b_256(
              ::scale::encode(val.committed_receipt.to_plain(*hasher)).value());
        },
        [&](const StatementWithPVDValid &val) { return val.candidate_hash; });
  }

  /**
   * @brief Converts a SignedFullStatementWithPVD to an IndexedAndSigned
   * CompactStatement.
   */
  inline IndexedAndSigned<network::vstaging::CompactStatement>
  signed_to_compact(const SignedFullStatementWithPVD &s,
                    const std::shared_ptr<crypto::Hasher> &hasher) {
    const Hash h = candidateHashFrom(getPayload(s), hasher);
    return {
        .payload =
            {
                .payload = visit_in_place(
                    getPayload(s),
                    [&](const StatementWithPVDSeconded &)
                        -> network::vstaging::CompactStatement {
                      return network::vstaging::SecondedCandidateHash{
                          .hash = h,
                      };
                    },
                    [&](const StatementWithPVDValid &)
                        -> network::vstaging::CompactStatement {
                      return network::vstaging::ValidCandidateHash{
                          .hash = h,
                      };
                    }),
                .ix = s.payload.ix,
            },
        .signature = s.signature,
    };
  }

}  // namespace kagome::parachain
