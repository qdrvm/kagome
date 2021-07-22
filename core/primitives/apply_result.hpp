/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_APPLY_RESULT_HPP
#define KAGOME_CORE_PRIMITIVES_APPLY_RESULT_HPP

#include <boost/variant.hpp>

namespace kagome::primitives {

  enum class DispatchOutcome : uint8_t { SUCCESS = 0, FAIL = 1 };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const DispatchOutcome &outcome) {
    uint8_t r{static_cast<uint8_t>(outcome)};
    return s << r;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, DispatchOutcome &outcome) {
    uint8_t int_res = 0;
    s >> int_res;
    outcome = DispatchOutcome{int_res};
    return s;
  }

  // https://github.com/paritytech/substrate/blob/25eb50fea5cb5644e5bbc2cf709f6c0c8e53406a/core/sr-primitives/src/lib.rs#L662

  enum class ApplyError : uint8_t {
    /// Bad signature.
    BAD_SIGNATURE = 0,
    /// Nonce too low.
    STALE = 1,
    /// Nonce too high.
    FUTURE = 2,
    /// Sending account had too low a balance.
    CANT_PAY = 3,
    /// Block is full, no more extrinsics can be applied.
    FULL_BLOCK = 255,
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const ApplyError &error) {
    uint8_t r{static_cast<uint8_t>(error)};
    return s << r;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, ApplyError &error) {
    uint8_t int_err = 0;
    s >> int_err;
    error = ApplyError{int_err};
    return s;
  }

  using ApplyExtrinsicResult = boost::variant<DispatchOutcome, ApplyError>;

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_APPLY_RESULT_HPP
