/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/validation/babe_block_validator.hpp"

#include <algorithm>

#include <sr25519/sr25519.h>
#include <boost/assert.hpp>
#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus,
                            BabeBlockValidator::ValidationError,
                            e) {
  using E = kagome::consensus::BabeBlockValidator::ValidationError;
  switch (e) {
    case E::NO_AUTHORITIES:
      return "no authorities are provided for the validation";
    case E::INVALID_DIGESTS:
      return "the block does not contain valid BABE digests";
    case E::INVALID_SIGNATURE:
      return "SR25519 signature, which is in BABE header, is invalid";
    case E::INVALID_VRF:
      return "VRF value and output are invalid";
    case E::TWO_BLOCKS_IN_SLOT:
      return "peer tried to distribute several blocks in one slot";
  }
  return "unknown error";
}

namespace kagome::consensus {
  BabeBlockValidator::BabeBlockValidator(
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<hash::Hasher> hasher,
      common::Logger log)
      : block_tree_{std::move(block_tree)},
        hasher_{std::move(hasher)},
        log_{std::move(log)} {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(log_);
  }

  outcome::result<void> BabeBlockValidator::validate(
      const primitives::Block &block,
      const PeerId &peer,
      gsl::span<const Authority> authorities) {
    if (authorities.empty()) {
      return ValidationError::NO_AUTHORITIES;
    }

    // get BABE-specific digests, which must be inside of this block
    auto digests_res = getBabeDigests(block);
    if (digests_res) {
      return digests_res.error();
    }
    auto [seal, babe_header] = digests_res.value();

    // signature in seal of the header must be valid
    if (!verifySignature(block, babe_header, seal, peer, authorities)) {
      return ValidationError::INVALID_SIGNATURE;
    }

    // VRF must prove that the peer is the leader of the slot
    if (!verifyVRF(babe_header)) {
      return ValidationError::INVALID_VRF;
    }

    // peer must not send two blocks in one slot
    if (!memorizeProducer(peer, babe_header.slot_number)) {
      return ValidationError::TWO_BLOCKS_IN_SLOT;
    }

    // all transactions in the block must be valid

    // there must exist a chain with the block in our storage, which is
    // specified as a parent of the block we are validating; BlockTree takes
    // care of this check and returns a specific error, if it fails
    return block_tree_->addBlock(block);
  }

  outcome::result<std::pair<Seal, BabeBlockHeader>>
  BabeBlockValidator::getBabeDigests(const primitives::Block &block) const {
    // valid BABE block has at least two digests: BabeHeader and a seal
    if (block.header.digests.size() < 2) {
      return ValidationError::INVALID_DIGESTS;
    }
    const auto &digests = block.header.digests;

    // last digest of the block must be a seal - signature
    auto seal_res = scale::decode<Seal>(digests.back());
    if (!seal_res) {
      return ValidationError::INVALID_DIGESTS;
    }

    for (const auto &digest : digests) {
      if (auto header = scale::decode<BabeBlockHeader>(digest)) {
        // found the BabeBlockHeader digest; return
        return {std::move(seal_res.value()), std::move(header.value())};
      }
    }

    return ValidationError::INVALID_DIGESTS;
  }

  bool BabeBlockValidator::verifySignature(
      const primitives::Block &block,
      const BabeBlockHeader &babe_header,
      const Seal &seal,
      const PeerId &peer,
      gsl::span<const Authority> authorities) const {
    // firstly, take hash of the block's header without Seal, which is the last
    // digest
    auto block_copy = block;
    block_copy.header.digests.pop_back();

    auto block_copy_encoded_res = scale::encode(block_copy.header);
    if (!block_copy_encoded_res) {
      log_->error("could not encode block header: {}",
                  block_copy_encoded_res.error());
      return false;
    }
    auto block_hash = hasher_->blake2s_256(block_copy_encoded_res.value());

    // secondly, retrieve public key of the peer by its authority id
    if (static_cast<uint64_t>(authorities.size())
        < babe_header.authority_index) {
      log_->error("don't know about authority with index {}",
                  babe_header.authority_index);
      return false;
    }
    const auto &key = authorities[babe_header.authority_index];

    // thirdly, use verify function to check the signature
    return sr25519_verify(seal.signature.data(),
                          block_hash.data(),
                          block_hash.size(),
                          key.id.data());
  }

  bool BabeBlockValidator::verifyVRF(const BabeBlockHeader &babe_header) const {
  }

  bool BabeBlockValidator::verifyProducer(const PeerId &peer,
                                          BabeSlotNumber number) {
    auto slot_it = blocks_producers_.find(number);
    if (slot_it == blocks_producers_.end()) {
      // this peer is the first producer in this slot
      blocks_producers_.insert({number, std::unordered_set<PeerId>{peer}});
      return true;
    }

    auto &slot = slot_it->second;
    auto peer_in_slot = slot.find(peer);
    if (peer_in_slot != slot.end()) {
      // this peer has already produced a block in this slot
      return false;
    }

    // OK
    slot.insert(peer);
    return true;
  }
}  // namespace kagome::consensus
