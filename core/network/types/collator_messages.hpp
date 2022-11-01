/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COLLATOR_DECLARE_HPP
#define KAGOME_COLLATOR_DECLARE_HPP

#include <boost/variant.hpp>
#include <scale/bitvec.hpp>
#include <tuple>
#include <type_traits>
#include <vector>

#include "common/blob.hpp"
#include "consensus/grandpa/common.hpp"
#include "crypto/hasher.hpp"
#include "primitives/common.hpp"
#include "primitives/compact_integer.hpp"
#include "primitives/digest.hpp"
#include "scale/tie.hpp"
#include "storage/trie/types.hpp"

namespace kagome::network {
  using Signature = crypto::Sr25519Signature;
  using ParachainId = uint32_t;
  using CollatorPublicKey = crypto::Sr25519PublicKey;
  using ValidatorIndex = uint32_t;
  using UpwardMessage = common::Buffer;
  using ParachainRuntime = common::Buffer;
  using HeadData = common::Buffer;
  using CandidateHash = primitives::BlockHash;
  using ChunkProof = std::vector<common::Buffer>;

  /// Payload signed by validator.
  template <typename Payload>
  struct Signed {
    SCALE_TIE(3);

    static_assert(std::is_same_v<std::decay_t<Payload>, Payload>);

    /// Payload.
    Payload payload;
    /// Index of validator in validator list.
    ValidatorIndex validator_index;
    /// Signature of `SigningContext::signable(payload)`.
    Signature signature;
  };

  /// NU element.
  using Dummy = std::tuple<>;

  /// ViewUpdate message. Maybe will be implemented later.
  using ViewUpdate = Dummy;

  /**
   * Collator -> Validator message.
   * Advertisement of a collation.
   */
  struct CollatorAdvertisement {
    SCALE_TIE(1);

    primitives::BlockHash relay_parent;  /// Hash of the parachain block.
  };

  /**
   * Collator -> Validator message.
   * Declaration of the intent to advertise a collation.
   */
  struct CollatorDeclaration {
    SCALE_TIE(3);

    CollatorPublicKey collator_id;  /// Public key of the collator.
    ParachainId para_id;            /// Parachain Id.
    Signature signature;  /// Signature of the collator using the PeerId of the
    /// collators node.
  };

  /// A chunk of erasure-encoded block data.
  struct ErasureChunk {
    SCALE_TIE(3);

    /// The erasure-encoded chunk of data belonging to the candidate block.
    common::Buffer chunk;
    /// The index of this erasure-encoded chunk of data.
    ValidatorIndex index;
    /// Proof for this chunk's branch in the Merkle tree.
    ChunkProof proof;
  };

  /**
   * PoV
   */
  struct ParachainBlock {
    SCALE_TIE(1);

    common::Buffer payload;  /// Contains the necessary data to for parachain
    /// specific state transition logic
  };

  /**
   * Unique descriptor of a candidate receipt.
   */
  struct CandidateDescriptor {
    SCALE_TIE(9);

    ParachainId para_id;  /// Parachain Id
    primitives::BlockHash
        relay_parent;  /// Hash of the relay chain block the candidate is
    /// executed in the context of
    CollatorPublicKey collator_id;  /// Collators public key.
    primitives::BlockHash
        persisted_data_hash;         /// Hash of the persisted validation data
    primitives::BlockHash pov_hash;  /// Hash of the PoV block.
    storage::trie::RootHash
        erasure_encoding_root;  /// Root of the block’s erasure encoding Merkle
    /// tree.
    Signature signature;  /// Collator signature of the concatenated components
    primitives::BlockHash
        para_head_hash;  /// Hash of the parachain head data of this candidate.
    primitives::BlockHash para_runtime_hash;  /// Hash of the parachain Runtime.

    common::Buffer signable() const {
      return common::Buffer{
          scale::encode(relay_parent,
                        para_id,
                        persisted_data_hash,
                        pov_hash,
                        para_runtime_hash)
              .value(),
      };
    }
  };

  /**
   * Contains information about the candidate and a proof of the results of its
   * execution.
   */
  struct CandidateReceipt {
    SCALE_TIE(2);

    CandidateDescriptor descriptor;          /// Candidate descriptor
    primitives::BlockHash commitments_hash;  /// Hash of candidate commitments
  };

  struct CollationResponse {
    SCALE_TIE(2);

    CandidateReceipt receipt;  /// Candidate receipt
    ParachainBlock pov;        /// PoV block
  };

  using ReqCollationResponseData = boost::variant<CollationResponse>;

  /**
   * Sent by clients who want to retrieve the advertised collation at the
   * specified relay chain block.
   */
  struct CollationFetchingRequest {
    SCALE_TIE(2);

    primitives::BlockHash relay_parent;  /// Hash of the relay chain block
    ParachainId para_id;                 /// Parachain Id.
  };

  /**
   * Sent by nodes to the clients who issued a collation fetching request.
   */
  struct CollationFetchingResponse {
    SCALE_TIE(1);

    ReqCollationResponseData response_data;  /// Response data
  };

  struct OutboundHorizontal {
    SCALE_TIE(2);

    ParachainId para_id;       /// Parachain Id is recepient id
    UpwardMessage upward_msg;  /// upward message for parallel parachain
  };

  struct CandidateCommitments {
    SCALE_TIE(6);

    std::vector<UpwardMessage> upward_msgs;  /// upward messages
    std::vector<OutboundHorizontal>
        outbound_hor_msgs;  /// outbound horizontal messages
    std::optional<ParachainRuntime>
        opt_para_runtime;          /// new parachain runtime if present
    HeadData para_head;            /// parachain head data
    uint32_t downward_msgs_count;  /// number of downward messages that were
    /// processed by the parachain
    uint32_t watermark;  /// watermark which specifies the relay chain block
    /// number up to which all inbound horizontal messages
    /// have been processed
  };

  struct CommittedCandidateReceipt {
    SCALE_TIE(2);

    CandidateDescriptor descriptor;    /// Candidate descriptor
    CandidateCommitments commitments;  /// commitments retrieved from validation
    /// result and produced by the execution
    /// and validation parachain candidate
  };

  using CandidateState = boost::variant<
      Dummy,                      /// not used
      CommittedCandidateReceipt,  /// Candidate receipt. Should be sent if
      /// validator seconded the candidate
      primitives::BlockHash  /// validator has deemed the candidate valid and
                             /// send the candidate hash
      >;

  struct Statement {
    SCALE_TIE(3);

    CandidateState candidate_state;
    ValidatorIndex validator_ix;
    Signature signature;
  };

  struct Seconded {
    SCALE_TIE(2);

    primitives::BlockHash relay_parent;  /// relay parent hash
    Statement statement;                 /// statement of seconded candidate
  };

  /// Signed availability bitfield.
  using SignedBitfield = Signed<scale::BitVec>;

  /**
   * Collator -> Validator and Validator -> Collator if statement message.
   * Type of the appropriate message.
   */
  using CollationMessage = boost::variant<
      CollatorDeclaration,    /// collator -> validator. Declare collator.
      CollatorAdvertisement,  /// collator -> validator. Make advertisement of
      /// the collation
      Dummy,    /// not used
      Dummy,    /// not used
      Seconded  /// validator -> collator. Candidate was seconded.
      >;

  /**
   * Collation protocol message.
   */
  using ProtocolMessage =
      boost::variant<CollationMessage  /// collation protocol message
                     >;

  /**
   * Common WireMessage that represents messages in NetworkBridge.
   */
  using WireMessage = boost::variant<Dummy,            /// not used
                                     ProtocolMessage,  /// protocol message
                                     ViewUpdate        /// view update message
                                     >;

  inline CandidateHash candidateHash(const crypto::Hasher &hasher,
                                     const CandidateReceipt &receipt) {
    return hasher.blake2b_256(scale::encode(receipt).value());
  }

  inline CandidateHash candidateHash(const crypto::Hasher &hasher,
                                     const CommittedCandidateReceipt &receipt) {
    return candidateHash(
        hasher,
        CandidateReceipt{
            receipt.descriptor,
            hasher.blake2b_256(scale::encode(receipt.commitments).value()),
        });
  }

  inline CandidateHash candidateHash(const crypto::Hasher &hasher,
                                     const CandidateState &statement) {
    if (auto receipt = boost::get<CommittedCandidateReceipt>(&statement)) {
      return candidateHash(hasher, *receipt);
    }
    return boost::get<CandidateHash>(statement);
  }
}  // namespace kagome::network

#endif  // KAGOME_COLLATOR_DECLARE_HPP
