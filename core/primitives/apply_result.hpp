/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>

#include "primitives/arithmetic_error.hpp"
#include "primitives/token_error.hpp"
#include "primitives/transaction_validity.hpp"

namespace kagome::primitives {

  // Implementation according to
  // https://github.com/paritytech/substrate/blob/7dcc77b982f59eaf6cec19499d981164b04a255d/primitives/runtime/src/lib.rs#L459
  // and
  // https://w3f-research.readthedocs.io/en/latest/_static/pdfview/viewer.html?file=https://w3f.github.io/polkadot-spec/spec/host/nightly.pdf#label329

  class DispatchSuccess {};

  namespace dispatch_error {
    /// Some unclassified error occurred.
    struct Other {
      std::string value;
    };

    /// Failed to lookup some data.
    struct CannotLookup {};
    /// A bad origin.
    struct BadOrigin {};
    /// A custom error in a module.
    struct Module {
      /// Module index, matching the metadata module index.
      uint8_t index;
      /// Module specific error value.
      uint8_t error;
      /// Optional error message.
      std::optional<std::string>
          message;  // not currently used in rust impl, thus not scale encoded
      SCALE_CUSTOM_DECOMPOSITION(Module, index, error);
    };

    /// At least one consumer is remaining so the account cannot be destroyed.
    struct ConsumerRemaining {};
    /// There are no providers so the account cannot be created.
    struct NoProviders {};
    /// An error to do with tokens.
    struct Token {
      TokenError error;
    };

    /// An arithmetic error.
    struct Arithmetic {
      ArithmeticError error;
    };
  }  // namespace dispatch_error

  namespace de = dispatch_error;
  using DispatchError = boost::variant<de::Other,
                                       de::CannotLookup,
                                       de::BadOrigin,
                                       de::Module,
                                       de::ConsumerRemaining,
                                       de::NoProviders,
                                       de::Token,
                                       de::Arithmetic>;

  using DispatchOutcome = boost::variant<DispatchSuccess, DispatchError>;

  using ApplyExtrinsicResult =
      boost::variant<DispatchOutcome, TransactionValidityError>;

}  // namespace kagome::primitives
