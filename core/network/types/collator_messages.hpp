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
#include "primitives/block_header.hpp"
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
  using CandidateIndex = uint32_t;
  using CoreIndex = uint32_t;
  using CandidateHash = primitives::BlockHash;

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

  struct SignedStatement {
    SCALE_TIE(2);

    primitives::BlockHash relay_parent;  /// relay parent hash
    Statement statement;                 /// statement of seconded candidate
  };

  /// Seconded is an alias for signed statement.
  using Seconded = SignedStatement;

  struct BitfieldData {
    SCALE_TIE(3);

    std::vector<bool> bitfield;   /// The availability bitfield
    ValidatorIndex validator_ix;  /// Validator index in the authority set.
    Signature signature;          /// Signature of the validator.
  };

  struct BitfieldDistribution {
    SCALE_TIE(2);

    primitives::BlockHash relay_parent;  /// Hash of the relay chain block
    BitfieldData data;
  };

  struct StatementMetadata {
    SCALE_TIE(4);

    primitives::BlockHash relay_parent;  /// Hash of the relay chain block
    primitives::BlockHash
        candidate_hash;           /// Hash of candidate that was used create the
                                  /// `CommitedCandidateRecept`.
    ValidatorIndex validator_ix;  /// Validator index in the authority set.
    Signature signature;          /// Signature of the validator.
  };

  using StatementDistributionMessage =
      boost::variant<SignedStatement, StatementMetadata>;

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

  /// An assignment story based on the VRF that authorized the relay-chain block
  /// where the candidate was included combined with a sample number.
  ///
  /// The context used to produce bytes is [`RELAY_VRF_MODULO_CONTEXT`]
  struct RelayVRFModulo {
    SCALE_TIE(1);

    uint32_t sample;  /// The sample number used in this cert.
  };

  /// An assignment story based on the VRF that authorized the relay-chain block
  /// where the candidate was included combined with the index of a particular
  /// core.
  ///
  /// The context is [`RELAY_VRF_DELAY_CONTEXT`]
  struct RelayVRFDelay {
    SCALE_TIE(1);

    CoreIndex core_index;  /// The core index chosen in this cert.
  };

  /// Different kinds of input data or criteria that can prove a validator's
  /// assignment to check a particular parachain.
  using AssignmentCertKind = boost::variant<RelayVRFModulo, RelayVRFDelay>;

  /// A certification of assignment.
  struct AssignmentCert {
    SCALE_TIE(2);

    AssignmentCertKind
        kind;  /// The criterion which is claimed to be met by this cert.
    crypto::VRFOutput vrf;  /// The VRF showing the criterion is met.
  };

  /// An assignment criterion which refers to the candidate under which the
  /// assignment is relevant by block hash.
  struct IndirectAssignmentCert {
    SCALE_TIE(3);

    primitives::BlockHash
        block_hash;            /// A block hash where the candidate appears.
    ValidatorIndex validator;  /// The validator index.
    AssignmentCert cert;       /// The cert itself.
  };

  /// A signed approval vote which references the candidate indirectly via the
  /// block.
  ///
  /// In practice, we have a look-up from block hash and candidate index to
  /// candidate hash, so this can be transformed into a `SignedApprovalVote`.
  struct IndirectSignedApprovalVote {
    SCALE_TIE(4);

    primitives::BlockHash
        block_hash;  /// A block hash where the candidate appears.
    CandidateIndex
        candidate_index;       /// The index of the candidate in the list of
                               /// candidates fully included as-of the block.
    ValidatorIndex validator;  /// The validator index.
    Signature signature;       /// The signature by the validator.
  };

  struct Assignment {
    SCALE_TIE(2);

    IndirectAssignmentCert indirect_assignment_cert;
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

    std::vector<BitfieldData>
        bitfields;  /// The array of signed bitfields by validators claiming the
                    /// candidate is available (or not). @note The array must be
                    /// sorted by validator index corresponding to the authority
                    /// set
    std::vector<Empty>
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

}  // namespace kagome::network

#endif  // KAGOME_COLLATOR_DECLARE_HPP
