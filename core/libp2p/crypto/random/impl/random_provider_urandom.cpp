/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/random/impl/random_provider_urandom.hpp"

#include <boost/filesystem.hpp>
#include <fstream>

#include "libp2p/crypto/error.hpp"

namespace libp2p::crypto::random {
  using kagome::common::Buffer;

  outcome::result<Buffer> RandomProviderURandom::randomBytes(
      size_t number) const {
    std::ifstream urandom;
    static const std::string_view path("/dev/urandom");
    if (not boost::filesystem::exists(path.data())) {
      return RandomProviderError::TOKEN_NOT_EXISTS;
    }

    urandom.open(path.data(), std::ios::binary | std::ios::in);
    if (!urandom.is_open()) {
      return RandomProviderError::FAILED_OPEN_FILE;
    }

    auto close_stream = gsl::finally([&urandom]() { urandom.close(); });

    std::vector<char> buffer(number, 0);
    urandom.read(buffer.data(), number);

    if (urandom.gcount() != number) {
      return RandomProviderError::FAILED_FETCH_BYTES;
    }

    Buffer out(number, 0);
    std::copy(out.begin(), out.end(), buffer.begin());

    return out;
  }
}  // namespace libp2p::crypto::random
