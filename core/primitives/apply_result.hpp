/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_APPLY_RESULT_HPP
#define KAGOME_CORE_PRIMITIVES_APPLY_RESULT_HPP

#include <boost/variant.hpp>
#include "common/variant_builder.hpp"
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
      // not currently used in rust impl, thus not scale encoded
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
      boost::optional<std::string>
          message;  // not currently used in rust impl, thus not scale encoded
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

  template <typename Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const DispatchError &v) {
    uint8_t index{static_cast<uint8_t>(v.which())};
    s << index;
    if (3 == index) {
      const auto &module = boost::get<de::Module>(v);
      s << module.index << module.error;
      // message field is omitted for compatibility with rust implementation
      // (there it is declared but omitted too)
    } else if (6 == index) {
      const auto &token = boost::get<de::Token>(v);
      s << static_cast<uint8_t>(token.error);
    } else if (7 == index) {
      const auto &arithmetic = boost::get<de::Arithmetic>(v);
      s << static_cast<uint8_t>(arithmetic.error);
    }
    return s;
  }

  template <typename Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, DispatchError &v) {
    uint8_t index = 0;
    s >> index;
    kagome::common::VariantBuilder builder(v);
    builder.init(index);
    if (3 == index) {
      auto &module = boost::get<de::Module>(v);
      s >> module.index >> module.error;
      // message field is omitted for compatibility with rust implementation
      // (there it is declared but omitted too)
    } else if (6 == index) {
      auto &token = boost::get<de::Token>(v);
      token.error = TokenError{index};
    } else if (7 == index) {
      auto &arithmetic = boost::get<de::Arithmetic>(v);
      arithmetic.error = ArithmeticError{index};
    }
    return s;
  }

  template <typename Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const DispatchOutcome &v) {
    uint8_t index{static_cast<uint8_t>(v.which())};
    s << index;
    if (1 == index) {  // DispatchError
      const auto &error = boost::get<DispatchError>(v);
      s << error;
    }
    return s;
  }

  template <typename Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, DispatchOutcome &v) {
    uint8_t index = 0;
    s >> index;
    kagome::common::VariantBuilder builder(v);
    builder.init(index);
    if (1 == index) {  // DispatchError
      auto &error = boost::get<DispatchError>(v);
      s >> error;
    }
    return s;
  }

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_APPLY_RESULT_HPP
