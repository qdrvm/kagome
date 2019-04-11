/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE16_HPP
#define KAGOME_BASE16_HPP

#include <optional>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"

/**
 * Encode/decode to/from base16 format
 */
namespace libp2p::multi::detail {

  /**
   * Encode bytes to base16 uppercase string
   * @param bytes to be encoded
   * @return encoded string
   */
  std::string encodeBase16Upper(const kagome::common::Buffer &bytes);
  /**
   * Encode bytes to base16 lowercase string
   * @param bytes to be encoded
   * @return encoded string
   */
  std::string encodeBase16Lower(const kagome::common::Buffer &bytes);

  /**
   * Decode base16 uppercase to bytes
   * @param string to be decoded
   * @return decoded bytes in case of success
   */
  outcome::result<kagome::common::Buffer> decodeBase16Upper(
      std::string_view string);
  /**
   * Decode base16 lowercase string to bytes
   * @param string to be decoded
   * @return decoded bytes in case of success
   */
  outcome::result<kagome::common::Buffer> decodeBase16Lower(
      std::string_view string);
}  // namespace libp2p::multi::detail

#endif  // KAGOME_BASE16_HPP
