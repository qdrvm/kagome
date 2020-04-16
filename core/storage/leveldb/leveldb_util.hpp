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

#ifndef KAGOME_LEVELDB_UTIL_HPP
#define KAGOME_LEVELDB_UTIL_HPP

#include <leveldb/status.h>
#include <gsl/span>
#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "storage/database_error.hpp"

namespace kagome::storage {

  template <typename T>
  inline outcome::result<T> error_as_result(const leveldb::Status &s) {
    if (s.IsNotFound()) {
      return DatabaseError::NOT_FOUND;
    }

    if (s.IsIOError()) {
      return DatabaseError::IO_ERROR;
    }

    if (s.IsInvalidArgument()) {
      return DatabaseError::INVALID_ARGUMENT;
    }

    if (s.IsCorruption()) {
      return DatabaseError::CORRUPTION;
    }

    if (s.IsNotSupportedError()) {
      return DatabaseError::NOT_SUPPORTED;
    }

    return DatabaseError::UNKNOWN;
  }

  template <typename T>
  inline outcome::result<T> error_as_result(const leveldb::Status &s,
                                            const common::Logger &logger) {
    logger->error(s.ToString());
    return error_as_result<T>(s);
  }

  inline leveldb::Slice make_slice(const common::Buffer &buf) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *ptr = reinterpret_cast<const char *>(buf.data());
    size_t n = buf.size();
    return leveldb::Slice{ptr, n};
  }

  inline gsl::span<const uint8_t> make_span(const leveldb::Slice &s) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *ptr = reinterpret_cast<const uint8_t *>(s.data());
    return gsl::make_span(ptr, s.size());
  }

  inline common::Buffer make_buffer(const leveldb::Slice &s) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *ptr = reinterpret_cast<const uint8_t *>(s.data());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return common::Buffer(ptr, ptr + s.size());
  }

}  // namespace kagome::storage

#endif  // KAGOME_LEVELDB_UTIL_HPP
