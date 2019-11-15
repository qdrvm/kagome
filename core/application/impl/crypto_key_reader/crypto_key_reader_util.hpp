/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_KEY_READER_UTIL_HPP
#define KAGOME_CRYPTO_KEY_READER_UTIL_HPP

#include <boost/filesystem.hpp>
#include <libp2p/crypto/key.hpp>
#include <outcome/outcome.hpp>

#include "common/buffer.hpp"

namespace kagome::application {

  /**
   * Reads a private key from PEM file.
   * Supported formats: Ed25519
   */
  outcome::result<common::Buffer> readPrivKeyFromPEM(
      const boost::filesystem::path &file, libp2p::crypto::Key::Type type);

  /**
   * Reads a HEX encoded keypair from a file. First there should be public key,
   * then, without separators, private key
   * @warning temporary solution, meant to be replaced by PEM file format
   */
  outcome::result<common::Buffer> readKeypairFromHexFile(
      const boost::filesystem::path &file);

  /**
   * Reads a HEX encoded private key from a file.
   * @warning temporary solution, meant to be replaced by PEM file format
   */
  outcome::result<common::Buffer> readPrivKeyFromHexFile(
      const boost::filesystem::path &file);

}  // namespace kagome::application

#endif  // KAGOME_CRYPTO_KEY_READER_UTIL_HPP
