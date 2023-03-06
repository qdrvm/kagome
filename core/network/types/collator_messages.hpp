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
#include "crypto/sr25519_types.hpp"
#include "parachain/approval/approval.hpp"
#include "parachain/types.hpp"
#include "primitives/block_header.hpp"
#include "primitives/common.hpp"
#include "primitives/compact_integer.hpp"
#include "primitives/digest.hpp"
#include "scale/tie.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {
  struct AvailableData;
}  // namespace kagome::runtime

namespace kagome::network {
  using namespace parachain;

  /// NU element.
  using Dummy = std::tuple<>;

  /// Empty element.
  using Empty = std::tuple<>;

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

  using RequestPov = CandidateHash;
  using ResponsePov = boost::variant<ParachainBlock, Empty>;

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
    primitives::BlockHash
        validation_code_hash;  /// Hash of the parachain Runtime.

    common::Buffer signable() const {
      return common::Buffer{
          scale::encode(relay_parent,
                        para_id,
                        persisted_data_hash,
                        pov_hash,
                        validation_code_hash)
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

    CandidateDescriptor descriptor;  /// Candidate descriptor
    Hash commitments_hash;           /// Hash of candidate commitments
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

    Hash relay_parent;    /// Hash of the relay chain block
    ParachainId para_id;  /// Parachain Id.
  };

  /**
   * Sent by nodes to the clients who issued a collation fetching request.
   */
  struct CollationFetchingResponse {
    SCALE_TIE(1);

    ReqCollationResponseData response_data;  /// Response data
  };

  /**
   * Sent by clients who want to retrieve chunks of a parachain candidate.
   */
  struct FetchChunkRequest {
    SCALE_TIE(2);

    CandidateHash candidate;  /// parachain candidate hash
    uint32_t chunk_index;     /// index of the chunk
  };

  /**
   * Sent by nodes to the clients who issued a chunk fetching request.
   */
  struct Chunk {
    SCALE_TIE(2);

    common::Buffer data;  /// chunk data
    ChunkProof proof;     /// chunk proof
  };
  using FetchChunkResponse = boost::variant<Chunk, Empty>;
  using FetchAvailableDataRequest = CandidateHash;
  using FetchAvailableDataResponse =
      boost::variant<runtime::AvailableData, Empty>;

  struct OutboundHorizontal {
    SCALE_TIE(2);

    ParachainId para_id;       /// Parachain Id is recepient id
    UpwardMessage upward_msg;  /// upward message for parallel parachain
  };

  struct InboundDownwardMessage {
    SCALE_TIE(2);
    /// The block number at which these messages were put into the downward
    /// message queue.
    BlockNumber sent_at;
    /// The actual downward message to processes.
    DownwardMessage msg;
  };

  struct InboundHrmpMessage {
    SCALE_TIE(2);
    /// The block number at which this message was sent.
    /// Specifically, it is the block number at which the candidate that sends
    /// this message was enacted.
    BlockNumber sent_at;
    /// The message payload.
    common::Buffer data;
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
    BlockNumber watermark;  /// watermark which specifies the relay chain block
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

  struct FetchStatementRequest {
    SCALE_TIE(2);
    RelayHash relay_parent;
    CandidateHash candidate_hash;
  };
  using FetchStatementResponse = boost::variant<CommittedCandidateReceipt>;

  struct ValidityAttestation {
    SCALE_TIE(2);

    struct Implicit : Empty {};
    struct Explicit : Empty {};

    boost::variant<Dummy, Implicit, Explicit> kind;
    Signature signature;
  };

  struct BackedCandidate {
    SCALE_TIE(3);

    CommittedCandidateReceipt candidate;
    std::vector<ValidityAttestation> validity_votes;
    scale::BitVec validator_indices;
  };

  using CandidateState =
      boost::variant<Dummy,                      /// not used
                     CommittedCandidateReceipt,  /// Candidate receipt. Should
                                                 /// be sent if
                     /// validator seconded the candidate
                     CandidateHash  /// validator has deemed the candidate valid
                                    /// and send the candidate hash
                     >;

  struct Statement {
    SCALE_TIE(1);
    CandidateState candidate_state;
  };
  using SignedStatement = IndexedAndSigned<Statement>;

  struct Seconded {
    SCALE_TIE(2);

    primitives::BlockHash relay_parent;  /// relay parent hash
    SignedStatement statement;           /// statement of seconded candidate
  };

  /// Signed availability bitfield.
  using SignedBitfield = parachain::IndexedAndSigned<scale::BitVec>;

  struct BitfieldDistribution {
    SCALE_TIE(2);

    primitives::BlockHash relay_parent;  /// Hash of the relay chain block
    SignedBitfield data;
  };

  struct StatementMetadata {
    SCALE_TIE(2);

    primitives::BlockHash relay_parent;  /// Hash of the relay chain block
    primitives::BlockHash
        candidate_hash;  /// Hash of candidate that was used create the
                         /// `CommitedCandidateRecept`.
  };

  /// A succinct representation of a peer's view. This consists of a bounded
  /// amount of chain heads and the highest known finalized block number.
  ///
  /// Up to `N` (5?) chain heads.
  /// The rust representation:
  /// https://github.com/paritytech/polkadot/blob/master/node/network/protocol/src/lib.rs#L160
  struct View {
    SCALE_TIE(2);

    /// A bounded amount of chain heads.
    /// Invariant: Sorted.
    std::vector<primitives::BlockHash> heads_;

    /// The highest known finalized block number.
    primitives::BlockNumber finalized_number_;

    bool contains(const primitives::BlockHash &hash) const {
      auto const it = std::lower_bound(heads_.begin(), heads_.end(), hash);
      return it != heads_.end() && *it == hash;
    }
  };

  struct ExView {
    View view;
    primitives::BlockHeader new_head;
    std::vector<primitives::BlockHash> lost;
  };

  using StatementDistributionMessage =
      boost::variant<Seconded, parachain::IndexedAndSigned<StatementMetadata>>;

  /**
   * Collator -> Validator and Validator -> Collator if seconded message.
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
   * Indicates the availability vote of a validator for a given candidate.
   */
  using BitfieldDistributionMessage = boost::variant<BitfieldDistribution>;

  /// A signed approval vote which references the candidate indirectly via the
  /// block.
  ///
  /// In practice, we have a look-up from block hash and candidate index to
  /// candidate hash, so this can be transformed into a `SignedApprovalVote`.
  struct ApprovalVote {
    SCALE_TIE(2);

    primitives::BlockHash
        block_hash;  /// A block hash where the candidate appears.
    CandidateIndex
        candidate_index;  /// The index of the candidate in the list of
                          /// candidates fully included as-of the block.
  };
  using IndirectSignedApprovalVote = parachain::IndexedAndSigned<ApprovalVote>;

  struct Assignment {
    SCALE_TIE(2);

    kagome::parachain::approval::IndirectAssignmentCert
        indirect_assignment_cert;
    CandidateIndex candidate_ix;
  };

  struct Assignments {
    SCALE_TIE(1);

    std::vector<Assignment> assignments;  /// Assignments for candidates in
                                          /// recent, unfinalized blocks.
  };

  struct Approvals {
    SCALE_TIE(1);

    std::vector<IndirectSignedApprovalVote>
        approvals;  /// Approvals for candidates in some recent, unfinalized
                    /// block.
  };

  using ApprovalDistributionMessage = boost::variant<Assignments, Approvals>;

  /// Attestation is either an implicit or explicit attestation of the validity
  /// of a parachain candidate, where 1 implies an implicit vote (in
  /// correspondence of a Seconded statement) and 2 implies an explicit
  /// attestation (in correspondence of a Valid statement). Both variants are
  /// followed by the signature of the validator.
  using Attestation = boost::variant<Unused<0>,                            // 0
                                     Tagged<Signature, struct Implicit>,   // 1
                                     Tagged<Signature, struct Explicit>>;  // 2

  struct CommittedCandidate {
    SCALE_TIE(3);

    CommittedCandidateReceipt
        candidate_receipt;  /// Committed candidate receipt
    std::vector<Attestation>
        validity_votes;  /// An array of validity votes themselves, expressed as
                         /// signatures
    std::vector<bool> indices;  /// A bitfield of indices of the validators
                                /// within the validator group
  };

  using DisputeStatement =
      boost::variant<Tagged<Empty, struct ExplicitStatement>,
                     Tagged<common::Hash256, struct SecondedStatement>,
                     Tagged<common::Hash256, struct ValidStatement>,
                     Tagged<Empty, struct AprovalVote>>;

  struct Vote {
    SCALE_TIE(3);

    uint32_t validator_index;  /// An unsigned 32-bit integer indicating the
                               /// validator index in the authority set
    Signature signature;       /// The signature of the validator
    DisputeStatement
        statement;  /// A varying datatype and implies the dispute statement
  };

  /// The dispute request is sent by clients who want to issue a dispute about a
  /// candidate.
  /// @details https://spec.polkadot.network/#net-msg-dispute-request
  struct DisputeRequest {
    SCALE_TIE(4);

    CommittedCandidateReceipt
        candidate;           /// The candidate that is being disputed
    uint32_t session_index;  /// An unsigned 32-bit integer indicating the
                             /// session index the candidate appears in
    Vote invalid_vote;       /// The invalid vote that makes up the request
    Vote valid_vote;  /// The valid vote that makes this dispute request valid
  };

  struct ParachainInherentData {
    SCALE_TIE(4);

    std::vector<SignedBitfield>
        bitfields;  /// The array of signed bitfields by validators claiming the
                    /// candidate is available (or not). @note The array must be
                    /// sorted by validator index corresponding to the authority
                    /// set
    std::vector<network::BackedCandidate>
        backed_candidates;  /// The array of backed candidates for inclusion in
                            /// the current block
    std::vector<DisputeRequest> disputes;  /// Array of disputes
    primitives::BlockHeader
        parent_header;  /// The head data is contains information about a
                        /// parachain block. The head data is returned by
                        /// executing the parachain Runtime and relay chain
                        /// validators are not concerned with its inner
                        /// structure and treat it as a byte arrays.
  };

  /**
   * Validator -> Validator.
   * Used by validators to broadcast relevant information about certain steps in
   * the A&V process.
   */
  using ValidatorProtocolMessage = boost::variant<
      Dummy,                         /// NU
      BitfieldDistributionMessage,   /// bitfield distribution message
      Dummy,                         /// NU
      StatementDistributionMessage,  /// statement distribution message
      ApprovalDistributionMessage    /// approval distribution message
      >;
  using CollationProtocolMessage = boost::variant<CollationMessage>;

  template <typename T, typename... AllowedTypes>
  struct AllowerTypeChecker {
    static constexpr bool allowed = (std::is_same_v<T, AllowedTypes> || ...);
  };

  using CompactStatementSeconded = primitives::BlockHash;
  using CompactStatementValid = primitives::BlockHash;

  /// Statements that can be made about parachain candidates. These are the
  /// actual values that are signed.
  using CompactStatement = boost::variant<
      Tagged<CompactStatementSeconded,
             struct SecondedTag>,  /// Proposal of a parachain candidate.
      Tagged<CompactStatementValid, struct ValidTag>  /// State that a parachain
                                                      /// candidate is valid.
      >;

  /// ViewUpdate message. Maybe will be implemented later.
  struct ViewUpdate {
    SCALE_TIE(1)
    View view;
  };

  /// Information about a core which is currently occupied.
  struct ScheduledCore {
    SCALE_TIE(2)

    /// The ID of a para scheduled.
    ParachainId para_id;
    /// The collator required to author the block, if any.
    std::optional<CollatorId> collator;
  };

  /**
   * Common WireMessage that represents messages in NetworkBridge.
   */
  template <typename T>
  using WireMessage = boost::variant<
      Dummy,  /// not used
      std::enable_if_t<AllowerTypeChecker<T,
                                          ValidatorProtocolMessage,
                                          CollationProtocolMessage>::allowed,
                       T>,  /// protocol message
      ViewUpdate            /// view update message
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

template <>
struct fmt::formatter<kagome::network::SignedBitfield> {
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }
    return it;
  }

  template <typename FormatContext>
  auto format(const kagome::network::SignedBitfield &val,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    char buf[8] = {0};
    auto const &bits = val.payload.payload.bits;

    static_assert(sizeof(buf) > 1, "Because of last zero-terminate symbol");
    size_t ix = 0;
    for (; ix < std::min(bits.size(), sizeof(buf) - size_t(1)); ++ix) {
      buf[ix] = bits[ix] ? '1' : '0';
    }

    return format_to(ctx.out(),
                     "sig={}, validator={}, bits=[0b{}{}]",
                     val.signature,
                     val.payload.ix,
                     buf,
                     ix == bits.size() ? "" : "…");
  }
};

#endif  // KAGOME_COLLATOR_DECLARE_HPP
