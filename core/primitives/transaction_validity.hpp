/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <vector>

#include <boost/variant.hpp>
#include <outcome/outcome.hpp>

#include "primitives/transaction.hpp"

namespace kagome::primitives {

  enum class TransactionSource : uint8_t {
    /// Transaction is already included in block.
    ///
    /// This means that we can't really tell where the transaction is coming
    /// from, since it's already in the received block. Note that the custom
    /// validation logic using either `Local` or `External` should most likely
    /// just allow `InBlock` transactions as well.
    InBlock,

    /// Transaction is coming from a local source.
    ///
    /// This means that the transaction was produced internally by the node
    /// (for instance an Off-Chain Worker, or an Off-Chain Call), as opposed
    /// to being received over the network.
    Local,

    /// Transaction has been received externally.
    ///
    /// This means the transaction has been received from (usually) "untrusted"
    /// source, for instance received over the network or RPC.
    External,
  };

  /**
   * @brief Information concerning a valid transaction.
   *
   * This is the same structure as in
   * https://github.com/paritytech/substrate/blob/a31c01b398d958ccf0a24d8c1c11fb073df66212/core/sr-primitives/src/transaction_validity.rs#L178
   */
  struct ValidTransaction {
    /**
     * @brief Priority of the transaction.
     * Priority determines the ordering of two transactions that have all
     * their dependencies (required tags) satisfied.
     */
    Transaction::Priority priority{};

    /**
     * @brief Transaction dependencies
     * A non-empty list signifies that some other transactions which provide
     * given tags are required to be included before that one.
     */
    std::vector<Transaction::Tag> required_tags;

    /**
     * @brief Provided tags
     * A list of tags this transaction provides. Successful transaction import
     * will enable other transactions that depend on (require) those tags to be
     * included as well. Provided and requried tags allow Substrate to build a
     * dependency graph of transactions and import them in the right (linear)
     * order.
     */
    std::vector<Transaction::Tag> provided_tags;

    /**
     * @brief Transaction longevity
     * Longevity describes minimum number of blocks the validity is correct.
     * After this period transaction should be removed from the pool or
     * revalidated.
     */
    Transaction::Longevity longevity{};

    /**
     * @brief A flag indicating if the transaction should be propagated to other
     * peers. By setting `false` here the transaction will still be considered
     * for including in blocks that are authored on the current node, but will
     * never be sent to other peers.
     */
    bool propagate{};

    bool operator==(const ValidTransaction &other) const = default;
  };

  /// Transaction is invalid. Details are described by the error code.
  struct InvalidTransaction {
    enum Kind : uint8_t {
      /// The call of the transaction is not expected.
      Call = 1,
      /// General error to do with the inability to pay some fees (e.g. account
      /// balance too low).
      Payment,
      /// General error to do with the transaction not yet being valid (e.g.
      /// nonce
      /// too high).
      Future,
      /// General error to do with the transaction being outdated (e.g. nonce
      /// too
      /// low).
      Stale,
      /// General error to do with the transaction's proofs (e.g. signature).
      BadProof,
      /// The transaction birth block is ancient.
      AncientBirthBlock,
      /// The transaction would exhaust the resources of current block.
      ///
      /// The transaction might be valid, but there are not enough resources
      /// left
      /// in the current block.
      ExhaustsResources,
      /// Any other custom invalid validity that is not covered by this enum.
      Custom,
      /// An extrinsic with a Mandatory dispatch resulted in Error. This is
      /// indicative of either a malicious validator or a buggy
      /// `provide_inherent`. In any case, it can result in dangerously
      /// overweight
      /// blocks and therefore if found, invalidates the block.
      BadMandatory,
      /// A transaction with a mandatory dispatch. This is invalid; only
      /// inherent
      /// extrinsics are allowed to have mandatory dispatches.
      MandatoryDispatch,
      /// The sending address is disabled or known to be invalid.
      BadSigner,
    };
    Kind kind{};
    uint8_t custom_value{};

    bool operator==(const InvalidTransaction &other) const {
      return kind == other.kind
          && (kind != Custom || custom_value == other.custom_value);
    }

    friend void encode(const InvalidTransaction &v, scale::Encoder &encoder) {
      // -1 is needed for compatibility with Rust; indices of error codes start
      // from 0 there, while in kagome they must start from 1 because of
      // std::error_code policy
      encoder.put(static_cast<uint8_t>(v.kind) - 1);
      if (v.kind == Custom) {
        encoder.put(v.custom_value);
      }
    }

    friend void decode(InvalidTransaction &v, scale::Decoder &decoder) {
      // increment is needed for compatibility with Rust; indices of error codes
      // start from 0 there, while in kagome they must start from 1 because of
      // std::error_code policy
      v.kind = static_cast<Kind>(decoder.take() + 1);
      if (v.kind == Custom) {
        v.custom_value = decoder.take();
      }
    }
  };

  /// An unknown transaction validity.
  struct UnknownTransaction {
    enum Kind : uint8_t {
      /// Could not lookup some information that is required to validate the
      /// transaction.
      CannotLookup = 1,
      /// No validator found for the given unsigned transaction.
      NoUnsignedValidator,
      /// Any other custom unknown validity that is not covered by this enum.
      Custom
    };

    bool operator==(Kind kind) const {
      return this->kind == kind;
    }

    bool operator==(const UnknownTransaction &) const = default;

    Kind kind;
    uint8_t custom_value{};

    friend void encode(const UnknownTransaction &v, scale::Encoder &encoder) {
      // -1 is needed for compatibility with Rust; indices of error codes start
      // from 0 there, while in kagome they must start from 1 because of
      // std::error_code policy
      encoder.put(static_cast<uint8_t>(v.kind) - 1);
      if (v.kind == Custom) {
        encoder.put(v.custom_value);
      }
    }

    friend void decode(UnknownTransaction &v, scale::Decoder &decoder) {
      // increment is needed for compatibility with Rust; indices of error codes
      // start from 0 there, while in kagome they must start from 1 because of
      // std::error_code policy
      v.kind = static_cast<Kind>(decoder.take() + 1);
      if (v.kind == Custom) {
        v.custom_value = decoder.take();
      }
    }
  };

  using TransactionValidityError =
      boost::variant<InvalidTransaction, UnknownTransaction>;

  /**
   * Information on a transaction's validity and, if valid, on how it relate to
   * other transactions.
   */
  using TransactionValidity =
      boost::variant<ValidTransaction, TransactionValidityError>;
}  // namespace kagome::primitives

OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, InvalidTransaction::Kind)
OUTCOME_HPP_DECLARE_ERROR(kagome::primitives, UnknownTransaction::Kind)
