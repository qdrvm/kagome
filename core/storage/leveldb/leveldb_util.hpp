/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LEVELDB_UTIL_HPP
#define KAGOME_LEVELDB_UTIL_HPP

#include <leveldb/status.h>
#include <gsl/span>
#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "common/logger.hpp"
#include "storage/leveldb/leveldb_error.hpp"

namespace kagome::storage {

  template <typename T>
  inline outcome::result<T> error_as_result(const leveldb::Status &s) {
    if (s.IsNotFound()) {
      return LevelDBError::NOT_FOUND;
    }

    if (s.IsIOError()) {
      return LevelDBError::IO_ERROR;
    }

    if (s.IsInvalidArgument()) {
      return LevelDBError::INVALID_ARGUMENT;
    }

    if (s.IsCorruption()) {
      return LevelDBError::CORRUPTION;
    }

    if (s.IsNotSupportedError()) {
      return LevelDBError::NOT_SUPPORTED;
    }

    return LevelDBError::UNKNOWN;
  }

  template <typename T>
  inline outcome::result<T> error_as_result(const leveldb::Status &s,
                                            const common::Logger &logger) {
    logger->error(s.ToString());
    return error_as_result<T>(s);
  }

  inline leveldb::Slice make_slice(const common::Buffer &buf) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *ptr = reinterpret_cast<const char *>(buf.toBytes());
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
