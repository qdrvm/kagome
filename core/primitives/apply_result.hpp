/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_APPLY_RESULT_HPP
#define KAGOME_CORE_PRIMITIVES_APPLY_RESULT_HPP

#include <boost/variant.hpp>
#include "primitives/arithmetic_error.hpp"
#include "primitives/token_error.hpp"
#include "primitives/transaction_validity.hpp"
#include "scale/scale.hpp"

namespace kagome::primitives {

  // Implementation according to
  // https://github.com/paritytech/substrate/blob/7dcc77b982f59eaf6cec19499d981164b04a255d/primitives/runtime/src/lib.rs#L459
  // and
  // https://w3f-research.readthedocs.io/en/latest/_static/pdfview/viewer.html?file=https://w3f.github.io/polkadot-spec/spec/host/nightly.pdf#label329

  class DispatchSuccess {};
  SCALE_EMPTY_CODER(DispatchSuccess);

  namespace dispatch_error {
    /// Some unclassified error occurred.
    struct Other {
      std::string value;
    };
    // string value is not currently encodes in rust implementation,
    // thus we use empty coder
    SCALE_EMPTY_CODER(Other);
    /// Failed to lookup some data.
    struct CannotLookup {};
    SCALE_EMPTY_CODER(CannotLookup);
    /// A bad origin.
    struct BadOrigin {};
    SCALE_EMPTY_CODER(BadOrigin);
    /// A custom error in a module.
    struct Module {
      /// Module index, matching the metadata module index.
      uint8_t index;
      /// Module specific error value.
      uint8_t error;
      /// Optional error message.
      boost::optional<std::string>
          message;  // not currently used in rust impl, thus not scale encoded
    };

    template <typename Stream,
              typename = std::enable_if_t<Stream::is_encoder_stream>>
    Stream &operator<<(Stream &s, const Module &v) {
      return s << v.index << v.error;
    }

    template <typename Stream,
              typename = std::enable_if_t<Stream::is_decoder_stream>>
    Stream &operator>>(Stream &s, Module &v) {
      s >> v.index >> v.error;
      return s;
    }

    /// At least one consumer is remaining so the account cannot be destroyed.
    struct ConsumerRemaining {};
    SCALE_EMPTY_CODER(ConsumerRemaining);
    /// There are no providers so the account cannot be created.
    struct NoProviders {};
    SCALE_EMPTY_CODER(NoProviders);
    /// An error to do with tokens.
    struct Token {
      TokenError error;
    };

    template <typename Stream,
              typename = std::enable_if_t<Stream::is_encoder_stream>>
    Stream &operator<<(Stream &s, const Token &v) {
      return s << v.error;
    }

    template <typename Stream,
              typename = std::enable_if_t<Stream::is_decoder_stream>>
    Stream &operator>>(Stream &s, Token &v) {
      s >> v.error;
      return s;
    }

    /// An arithmetic error.
    struct Arithmetic {
      ArithmeticError error;
    };

    template <typename Stream,
              typename = std::enable_if_t<Stream::is_encoder_stream>>
    Stream &operator<<(Stream &s, const Arithmetic &v) {
      return s << v.error;
    }

    template <typename Stream,
              typename = std::enable_if_t<Stream::is_decoder_stream>>
    Stream &operator>>(Stream &s, Arithmetic &v) {
      s >> v.error;
      return s;
    }

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

#endif  // KAGOME_CORE_PRIMITIVES_APPLY_RESULT_HPP
