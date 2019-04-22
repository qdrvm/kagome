/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONVERTER_UTILS_HPP
#define KAGOME_CONVERTER_UTILS_HPP

#include <string>
#include <vector>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/multi/multiaddress_protocol_list.hpp"

namespace libp2p::multi::converters {

  /**
   * Converts the given address string of the specified protocol
   * to a hex string that represents the address, if both
   * address and protocol were valid
   */
  auto addressToHex(const Protocol &protocol, std::string_view addr)
      -> outcome::result<std::string>;

  /**
   * Converts the given multiaddr string to a byte sequence representing
   * the multiaddr, if provided multiaddr was valid
   */
  auto multiaddrToBytes(std::string_view multiaddr_str)
      -> outcome::result<kagome::common::Buffer>;

  /**
   * Converts the given byte sequence representing
   * a multiaddr to a string containing the multiaddr in a human-readable
   * format, if the provided byte sequence was a valid multiaddr
   */
  auto bytesToMultiaddrString(const kagome::common::Buffer &bytes)
      -> outcome::result<std::string>;

}  // namespace libp2p::multi::converters

#endif  // KAGOME_CONVERTER_UTILS_HPP
