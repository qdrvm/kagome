/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/babe/impl/babe_digests_util.hpp"

#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, DigestError, e) {
  using E = kagome::consensus::DigestError;
  switch (e) {
    case E::INVALID_DIGESTS:
      return "the block does not contain valid BABE digests";
    case E::MULTIPLE_EPOCH_CHANGE_DIGESTS:
      return "the block contains multiple epoch change digests";
    case E::NEXT_EPOCH_DIGEST_DOES_NOT_EXIST:
      return "next epoch digest does not exist";
  }
  return "unknown error";
}

namespace kagome::consensus {

  outcome::result<std::pair<Seal, BabeBlockHeader>> getBabeDigests(
      const primitives::BlockHeader &block_header) {
    // valid BABE block has at least two digests: BabeHeader and a seal
    if (block_header.digest.size() < 2) {
      return DigestError::INVALID_DIGESTS;
    }
    const auto &digests = block_header.digest;

    // last digest of the block must be a seal - signature
    auto seal_res = getFromVariant<primitives::Seal>(digests.back());
    if (!seal_res) {
      return DigestError::INVALID_DIGESTS;
    }

    OUTCOME_TRY(babe_seal_res, scale::decode<Seal>(seal_res.value().data));

    for (const auto &digest :
         gsl::make_span(digests).subspan(0, digests.size() - 1)) {
      if (auto pre_runtime = getFromVariant<primitives::PreRuntime>(digest);
          pre_runtime) {
        if (auto header = scale::decode<BabeBlockHeader>(pre_runtime->data);
            header) {
          // found the BabeBlockHeader digest; return
          return {babe_seal_res, header.value()};
        }
      }
    }

    return DigestError::INVALID_DIGESTS;
  }

  outcome::result<EpochDigest> getNextEpochDigest(
      const primitives::BlockHeader &header) {
    // https://github.com/paritytech/substrate/blob/d8df977d024ebeb5330bacac64cf7193a7c242ed/core/consensus/babe/src/lib.rs#L497
    outcome::result<EpochDigest> epoch_digest =
        DigestError::NEXT_EPOCH_DIGEST_DOES_NOT_EXIST;

    for (const auto &log : header.digest) {
      visit_in_place(
          log,
          [&epoch_digest](const primitives::Consensus &consensus) {
            if (consensus.consensus_engine_id == primitives::kBabeEngineId) {
              auto consensus_log_res =
                scale::decode<primitives::Consensus::BabeDigest>(consensus.data);
              if (not consensus_log_res) {
                return;
              }

              visit_in_place(
                  consensus_log_res.value(),
                  [&epoch_digest](const primitives::NextEpochData &next_epoch) {
                    if (not epoch_digest) {
                      epoch_digest = static_cast<EpochDigest>(next_epoch);
                    } else {
                      epoch_digest = DigestError::MULTIPLE_EPOCH_CHANGE_DIGESTS;
                    }
                  },
                  [](const auto &) {});
            }
          },
          [](const auto &) {});
    }
    return epoch_digest;
  }
}  // namespace kagome::consensus
