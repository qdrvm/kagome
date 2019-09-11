/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_BLOCK_VALIDATOR_HPP
#define KAGOME_BABE_BLOCK_VALIDATOR_HPP

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "blockchain/block_tree.hpp"
#include "common/logger.hpp"
#include "consensus/babe/types/babe_block_header.hpp"
#include "consensus/babe/types/seal.hpp"
#include "consensus/validation/block_validator.hpp"
#include "crypto/hasher.hpp"
#include "crypto/vrf_provider.hpp"
#include "primitives/authority.hpp"
#include "runtime/tagged_transaction_queue.hpp"

namespace kagome::consensus {
  /**
   * Validation of blocks in BABE system. Based on the algorithm described here:
   * https://research.web3.foundation/en/latest/polkadot/BABE/Babe/#2-normal-phase
   */
  class BabeBlockValidator : public BlockValidator {
   public:
    /**
     * Create an instance of BabeBlockValidator
     * @param block_tree to be used by this instance
     * @param tx_queue to validate the extrinsics
     * @param hasher to take hashes
     * @param vrf_provider for VRF-specific operations
     * @param log to write info to
     */
    BabeBlockValidator(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<runtime::TaggedTransactionQueue> tx_queue,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<crypto::VRFProvider> vrf_provider,
        common::Logger log = common::createLogger("BabeBlockValidator"));

    ~BabeBlockValidator() override = default;

    enum class ValidationError {
      NO_AUTHORITIES = 1,
      INVALID_DIGESTS,
      INVALID_SIGNATURE,
      INVALID_VRF,
      TWO_BLOCKS_IN_SLOT,
      INVALID_TRANSACTIONS
    };

    outcome::result<void> validate(const primitives::Block &block,
                                   const Epoch &epoch) override;

   private:
    /**
     * Get Babe-specific digests, which must be in the block
     * @return digests or error
     */
    outcome::result<std::pair<Seal, BabeBlockHeader>> getBabeDigests(
        const primitives::Block &block) const;

    /**
     * Verify that \param babe_header contains valid SR25519 signature
     * @return true, if signature is valid, false otherwise
     */
    bool verifySignature(
        const primitives::Block &block,
        const BabeBlockHeader &babe_header,
        const Seal &seal,
        gsl::span<const primitives::Authority> authorities) const;

    /**
     * Verify that \param babe_header contains valid VRF output
     * @return true, if output is correct, false otherwise
     */
    bool verifyVRF(const BabeBlockHeader &babe_header,
                   const Epoch &epoch) const;

    /**
     * Check, if the peer has produced a block in this slot and memorize, if the
     * peer hasn't
     * @param peer to be checked
     * @return true, if the peer has not produced any blocks in this slot, false
     * otherwise
     */
    bool verifyProducer(const BabeBlockHeader &babe_header);

    /**
     * Check, if all transactions in the block are valid
     * @return true, if all transactions have passed verification, false
     * otherwise
     */
    bool verifyTransactions(const primitives::BlockBody &block_body) const;

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::unordered_map<BabeSlotNumber,
                       std::unordered_set<primitives::AuthorityIndex>>
        blocks_producers_;

    std::shared_ptr<runtime::TaggedTransactionQueue> tx_queue_;

    std::shared_ptr<crypto::Hasher> hasher_;

    std::shared_ptr<crypto::VRFProvider> vrf_provider_;

    common::Logger log_;
  };
}  // namespace kagome::consensus

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus,
                          BabeBlockValidator::ValidationError)

#endif  // KAGOME_BABE_BLOCK_VALIDATOR_HPP
