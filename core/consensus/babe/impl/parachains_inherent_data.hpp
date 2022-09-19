/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <scale/bitvec.hpp>

#include "common/stub.hpp"
#include "common/tagged.hpp"
#include "common/unused.hpp"
#include "network/types/collator_messages.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus::babe {
  using Signature = crypto::Ed25519Signature;  // FIXME check actual type
  using ParachainId = uint32_t;
  using CollatorPublicKey =
      crypto::Ed25519PublicKey;  // FIXME check actual type

  /// @details https://spec.polkadot.network/#defn-candidate-descriptor
  struct CandidateDescriptor {
    /// The parachain Id
    ParachainId parachain_id;

    /// The hash of the relay chain block the candidate is executed in the
    /// context of.
    primitives::BlockHash relay_chain_block_hash;

    /// The collators public key.
    CollatorPublicKey collator_public_key;

    /// The hash of the persisted validation data (Definition 188).
    common::Hash256 validation_data_hash;

    /// The hash of the PoV block.
    primitives::BlockHash pov_block_hash;

    /// The root of the block’s erasure encoding Merkle tree.
    common::Hash256 merkle;

    /// The collator signature of the concatenated components
    Signature collator_signature;

    /// The hash of the parachain head data (Section 6.8.4) of this candidate.
    common::Hash256 parachain_head_data_hash;

    /// The hash of the parachain Runtime.
    common::Hash256 parachain_runtime_hash;
  };

  scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                        const CandidateDescriptor &data) {
    // clang-format off
    return s << data.parachain_id
             << data.relay_chain_block_hash
             << data.collator_public_key
             << data.validation_data_hash
             << data.pov_block_hash
             << data.merkle
             << data.collator_signature
             << data.parachain_head_data_hash
             << data.parachain_runtime_hash;
    // clang-format on
  }

  scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                        CandidateDescriptor &data) {
    // clang-format off
    return s >> data.parachain_id
             >> data.relay_chain_block_hash
             >> data.collator_public_key
             >> data.validation_data_hash
             >> data.pov_block_hash
             >> data.merkle
             >> data.collator_signature
             >> data.parachain_head_data_hash
             >> data.parachain_runtime_hash;
    // clang-format on
  }

  /// The recipient Id as defined in Definition 7.5 (non existent definition)
  struct RecipientTag {};
  using Recipient = Stub<RecipientTag>;  // FIXME

  ///  An upward message as defined in Definition 7.8 (non existent definition)
  struct UpwardMessageTag {};
  using UpwardMessage = Stub<UpwardMessageTag>;  // FIXME

  /// @details https://spec.polkadot.network/#defn-outbound-hrmp-message
  struct OutboundHrmpMessage {
    /// The recipient Id
    Recipient recipient;

    ///  An upward message
    UpwardMessage message;
  };

  scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                        const OutboundHrmpMessage &data) {
    return s << data.recipient << data.message;
  }

  scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                        OutboundHrmpMessage &data) {
    return s >> data.recipient >> data.message;
  }

  /// @details https://spec.polkadot.network/#defn-candidate-commitments
  struct CandidateCommitments {
    /// An array of upward messages sent by the parachain. Each individual
    /// message, m, is an array of bytes.
    std::vector<common::Buffer> upward_messages;

    /// An array of individual outbound horizontal messages (Section 6.8.10)
    /// sent by the parachain.
    std::vector<OutboundHrmpMessage> outbound_messages;

    /// A new parachain Runtime in case of an update.
    std::optional<common::Buffer>
        new_parachain_runtime;  // FIXME check actual type

    /// The parachain head data (Section 6.8.4).
    common::Buffer parachain_head_data;

    /// A unsigned 32-bit integer indicating the number of downward messages
    /// that were processed by the parachain. It is expected that the parachain
    /// processes the messages from first to last.
    uint32_t number_of_downward_messages;

    /// A unsigned 32-bit integer indicating the watermark which specifies the
    /// relay chain block number up to which all inbound horizontal messages
    /// have been processed.
    uint32_t watermark;
  };

  scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                        const CandidateCommitments &data) {
    // clang-format off
    return s << data.upward_messages
             << data.outbound_messages
             << data.new_parachain_runtime
             << data.parachain_head_data
             << data.number_of_downward_messages
             << data.watermark;
    // clang-format on
  }

  scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                        CandidateCommitments &data) {
    // clang-format off
    return s >> data.upward_messages
             >> data.outbound_messages
             >> data.new_parachain_runtime
             >> data.parachain_head_data
             >> data.number_of_downward_messages
             >> data.watermark;
    // clang-format on
  }

  /// A candidate receipt, contains information about the candidate and a proof
  /// of the results of its execution
  /// @details https://spec.polkadot.network/#defn-candidate-receipt
  struct CandidateReceipt {
    CandidateDescriptor descriptor;
    CandidateCommitments commitments;
  };

  scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                        const CandidateReceipt &data) {
    return s << data.descriptor << data.commitments;
  }

  scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                        CandidateReceipt &data) {
    return s >> data.descriptor >> data.commitments;
  }

  /// Attestation is either an implicit or explicit attestation of the validity
  /// of a parachain candidate, where 1 implies an implicit vote (in
  /// correspondence of a Seconded statement) and 2 implies an explicit
  /// attestation (in correspondence of a Valid statement). Both variants are
  /// followed by the signature of the validator.
  using Attestation = boost::variant<Unused<0>,                            // 0
                                     Tagged<Signature, struct Implicit>,   // 1
                                     Tagged<Signature, struct Explicit>>;  // 2

  struct CommittedCandidate {
    /// Committed candidate receipt
    CandidateReceipt candidate;

    /// An array of validity votes themselves, expressed as signatures
    std::vector<Attestation> validity_votes;

    /// A bitfield of indices of the validators within the validator group
    scale::BitVec indices;
  };

  scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                        const CommittedCandidate &data) {
    // clang-format off
    return s << data.candidate
             << data.validity_votes
             << data.indices;
    // clang-format on
  }

  scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                        CommittedCandidate &data) {
    // clang-format off
    return s >> data.candidate
             >> data.validity_votes
             >> data.indices;
    // clang-format on
  }

  using DisputeStatement =
      boost::variant<Tagged<Empty, struct ExplicitStatement>,
                     Tagged<common::Hash256, struct SecondedStatement>,
                     Tagged<common::Hash256, struct ValidStatement>,
                     Tagged<Empty, struct AprovalVote>>;

  struct Vote {
    /// An unsigned 32-bit integer indicating the validator index in the
    /// authority set
    uint32_t validator_index;

    /// The signature of the validator
    Signature signature;

    /// A varying datatype and implies the dispute statement
    DisputeStatement statement;
  };

  scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                        const Vote &data) {
    // clang-format off
    return s << data.validator_index
             << data.signature
             << data.statement;
    // clang-format on
  }

  scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                        Vote &data) {
    // clang-format off
    return s >> data.validator_index
             >> data.signature
             >> data.statement;
    // clang-format on
  }

  /// The dispute request is sent by clients who want to issue a dispute about a
  /// candidate.
  /// @details https://spec.polkadot.network/#net-msg-dispute-request
  struct DisputeRequest {
    /// The candidate that is being disputed
    CandidateReceipt candidate;

    /// An unsigned 32-bit integer indicating the session index the candidate
    /// appears in
    uint32_t session_index;

    /// The invalid vote that makes up the request
    Vote invalid_vote;

    /// The valid vote that makes this dispute request valid
    Vote valid_vote;
  };

  scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                        const DisputeRequest &data) {
    // clang-format off
    return s << data.candidate
             << data.session_index
             << data.invalid_vote
             << data.valid_vote;
    // clang-format on
  }

  scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                        DisputeRequest &data) {
    // clang-format off
    return s >> data.candidate
             >> data.session_index
             >> data.invalid_vote
             >> data.valid_vote;
    // clang-format on
  }

  struct ParachainInherentData {
    /// The array of signed bitfields by validators claiming the candidate is
    /// available (or not).
    /// @note The array must be sorted by validator index corresponding to the
    /// authority set
    std::vector<network::SignedBitfield> bitfields;

    /// The array of backed candidates for inclusion in the current block
    std::vector<Empty> backed_candidates;

    /// Array of disputes
    std::vector<DisputeRequest> disputes;

    /// The head data is contains information about a parachain block.
    /// The head data is returned by executing the parachain Runtime and relay
    /// chain validators are not concerned with its inner structure and treat it
    /// as a byte arrays.
    primitives::BlockHeader parent_header;
  };

  scale::ScaleEncoderStream &operator<<(scale::ScaleEncoderStream &s,
                                        const ParachainInherentData &data) {
    // clang-format off
    return s << data.bitfields
             << data.backed_candidates
             << data.disputes
             << data.parent_header;
    // clang-format on
  }

  scale::ScaleDecoderStream &operator>>(scale::ScaleDecoderStream &s,
                                        ParachainInherentData &data) {
    // clang-format off
    return s >> data.bitfields
             >> data.backed_candidates
             >> data.disputes
             >> data.parent_header;
    // clang-format on
  }

}  // namespace kagome::consensus::babe
