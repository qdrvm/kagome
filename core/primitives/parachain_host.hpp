/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_PRIMITIVES_PARACHAIN_HOST_HPP
#define KAGOME_CORE_PRIMITIVES_PARACHAIN_HOST_HPP

#include <cstdint>
#include <vector>

#include <boost/optional.hpp>
#include <boost/variant.hpp>
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
  template <class Stream, typename = std::enable_if_t<Stream::is_encoder_stream>>
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
