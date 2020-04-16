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

#include "crypto/sha/sha256.hpp"

#include <openssl/sha.h>

namespace kagome::crypto {
  common::Hash256 sha256(std::string_view input) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *bytes_ptr = reinterpret_cast<const uint8_t *>(input.data());
    return sha256(gsl::make_span(bytes_ptr, input.length()));
  }

  common::Hash256 sha256(gsl::span<const uint8_t> input) {
    common::Hash256 out;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, input.data(), input.size());
    SHA256_Final(out.data(), &ctx);
    return out;
  }
}  // namespace kagome::crypto
