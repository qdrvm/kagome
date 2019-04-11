/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BASE58_HPP
#define KAGOME_BASE58_HPP

#include <optional>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"

/**
 * Encode/decode to/from base58 format
 * Implementation is taken from
 * https://github.com/bitcoin/bitcoin/blob/master/src/base58.h
 */
namespace libp2p::multi::detail {

  /**
   * Encode bytes to base58 string
   * @param bytes to be encoded
   * @return encoded string
   */
  std::string encodeBase58(const kagome::common::Buffer &bytes);

  /**
   * Decode base58 string to bytes
   * @param string to be decoded
   * @return decoded bytes in case of success
   */
  outcome::result<kagome::common::Buffer> decodeBase58(
      std::string_view string);
}  // namespace libp2p::multi::detail

#endif  // KAGOME_BASE58_HPP
