/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_PARACHAIN_HOST_HPP
#define KAGOME_CORE_PRIMITIVES_PARACHAIN_HOST_HPP

#include <cstdint>
#include <vector>

#include <boost/variant.hpp>
#include <optional>
#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::primitives::parachain {
  /**
   * @brief ValidatorId primitive is an ed25519 or sr25519 public key
   */
  using ValidatorId = common::Blob<32>;

  /**
   * @brief ParachainId primitive is an uint32_t
   */
  using ParaId = uint32_t;

  /**
   * @brief Relay primitive is empty in polkadot for now
   */
  struct Relay {};
  // TODO(yuraz): PRE-152 update Relay content when it updates in polkadot

  /**
   * @brief Parachain primitive
   */
  using Parachain = ParaId;

  /**
   * @brief Chain primitive
   */
  using Chain = boost::variant<Relay, Parachain>;

  /**
   * @brief DutyRoster primitive
   */
  using DutyRoster = std::vector<Chain>;
  // TODO(yuraz): PRE-152 update Relay << and >> operators
  //  when Relay updates in polkadot
  /**
   * @brief outputs Relay instance to stream
   * @tparam Stream stream type
   * @param s reference to stream
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Relay &v) {
    return s;
  }

  /**
   * @brief decodes Relay instance from stream
   * @tparam Stream input stream type
   * @param s reference to stream
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Relay &v) {
    return s;
  }
}  // namespace kagome::primitives::parachain
#endif  // KAGOME_CORE_PRIMITIVES_PARACHAIN_HOST_HPP
