/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/sha/sha256.hpp"

#include <iomanip>
#include <sstream>

#include <openssl/sha.h>

namespace kagome::crypto {
  common::Buffer sha256(std::string_view str) {
    return sha256(common::Buffer{}.put(str));
  }

  common::Buffer sha256(const common::Buffer &bytes) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, bytes.toBytes(), bytes.size());
    SHA256_Final(static_cast<unsigned char *>(hash), &sha256);

    std::stringstream ss;
    for (auto i : hash) {
      ss << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(i);
    }
    return common::Buffer{}.putBytes(std::begin(hash), std::end(hash));
  }
}  // namespace kagome::crypto
