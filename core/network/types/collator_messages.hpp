/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COLLATOR_DECLARE_HPP
#define KAGOME_COLLATOR_DECLARE_HPP

#include <boost/variant.hpp>
#include <tuple>
#include <type_traits>
#include <vector>

#include "common/blob.hpp"
#include "consensus/grandpa/common.hpp"
#include "primitives/common.hpp"
#include "primitives/compact_integer.hpp"
#include "primitives/digest.hpp"
#include "scale/tie.hpp"
#include "storage/trie/types.hpp"

namespace kagome::network {
  using Signature = crypto::Sr25519Signature;
  using ParachainId = uint32_t;
  using CollatorPublicKey = crypto::Sr25519PublicKey;

  /**
   * Possible collation message contents.
   * [X,Y] - possible values.
   * [0] --- 0 ---> [0, 1, 4] -+- 0 ---> struct
   * CollatorDeclaration(collator_pubkey, para_id, collator_signature)
   *                           |- 1 ---> struct CollatorAdvertisement(para_hash)
   *                           |- 4 ---> struct CollatorSeconded(relay_hash,
   * CollatorStatement)
   */

  using Dummy = std::tuple<>;

  /**
   * Collator -> Validator message.
   * Advertisement of a collation.
   */
  struct CollatorAdvertisement {
    SCALE_TIE(1);
    SCALE_TIE_EQ(CollatorAdvertisement);

    /*
     * Hash of the parachain block.
     */
    primitives::BlockHash para_hash;
  };

  /**
   * Collator -> Validator message.
   * Declaration of the intent to advertise a collation.
   */
  struct CollatorDeclaration {
    SCALE_TIE(3);
    SCALE_TIE_EQ(CollatorDeclaration);

    /*
     * Public key of the collator.
     */
    consensus::grandpa::Id collator_pubkey;

    /*
     * Parachain Id.
     */
    uint32_t para_id;

    /*
     * Signature of the collator using the PeerId of the collators node.
     */
    consensus::grandpa::Signature collator_signature;
  };

  /**
   * Collator -> Validator and Validator -> Collator if statement message.
   * Type of the appropriate message.
   * [4] will be replaced to Statement
   */
  using CollationMessage =
      boost::variant<CollatorDeclaration, CollatorAdvertisement>;

  /**
   * Collation protocol message.
   */
  using ProtocolMessage = boost::variant<CollationMessage>;

  /**
   * ViewUpdate message. Maybe will be implemented later.
   */
  using ViewUpdate = Dummy;

  /**
   * Common WireMessage that represents messages in NetworkBridge.
   */
  using WireMessage = boost::variant<Dummy, ProtocolMessage, ViewUpdate>;

  /**
   * PoV
   */
  using ParachainBlock =
      std::tuple<common::Buffer  /// Contains the necessary data to for
                                 /// parachain specific state transition logic
                 >;

  /**
   * Unique descriptor of a candidate receipt.
   */
  using CandidateDescriptor = std::tuple<
      uint32_t,               /// Parachain Id
      primitives::BlockHash,  /// Hash of the relay chain block the candidate is
                              /// executed in the context of
      consensus::grandpa::Id,   /// Collators public key.
      primitives::BlockHash,    /// Hash of the persisted validation data
      primitives::BlockHash,    /// Hash of the PoV block.
      storage::trie::RootHash,  /// Root of the block’s erasure encoding Merkle
                                /// tree.
      consensus::grandpa::Signature,  /// Collator signature of the concatenated
                                      /// components
      primitives::BlockHash,  /// Hash of the parachain head data of this
                              /// candidate.
      primitives::BlockHash   /// Hash of the parachain Runtime.
      >;

  /**
   * Contains information about the candidate and a proof of the results of its
   * execution.
   */
  using CandidateReceipt =
      std::tuple<CandidateDescriptor,   /// Candidate descriptor
                 primitives::BlockHash  /// Hash of candidate commitments
                 >;

  using CollationResponse = std::tuple<CandidateReceipt,  /// Candidate receipt
                                       ParachainBlock     /// PoV block
                                       >;

  using ReqCollationResponseData = boost::variant<CollationResponse>;

  /**
   * Sent by clients who want to retrieve the advertised collation at the
   * specified relay chain block.
   */
  using CollationFetchingRequest =
      std::tuple<primitives::BlockHash,  /// Hash of the relay chain block
                 uint32_t                /// Parachain Id.
                 >;

  /**
   * Sent by nodes to the clients who issued a collation fetching request.
   */
  using CollationFetchingResponse =
      std::tuple<ReqCollationResponseData  /// Response data
                 >;

}  // namespace kagome::network

#endif  // KAGOME_COLLATOR_DECLARE_HPP
