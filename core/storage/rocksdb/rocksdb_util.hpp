/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <rocksdb/status.h>
#include "common/buffer.hpp"
#include "storage/database_error.hpp"

namespace kagome::storage {
  inline DatabaseError status_as_error(const rocksdb::Status &s) {
    if (s.IsNotFound()) {
      return DatabaseError::NOT_FOUND;
    }

    if (s.IsIOError()) {
      static log::Logger log = log::createLogger("RocksDb", "storage");
      SL_ERROR(log, ":{}", s.ToString());
      return DatabaseError::IO_ERROR;
    }

    if (s.IsInvalidArgument()) {
      return DatabaseError::INVALID_ARGUMENT;
    }

    if (s.IsCorruption()) {
      return DatabaseError::CORRUPTION;
    }

    if (s.IsNotSupported()) {
      return DatabaseError::NOT_SUPPORTED;
    }

    return DatabaseError::UNKNOWN;
  }

  inline rocksdb::Slice make_slice(const common::BufferView &buf) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *ptr = reinterpret_cast<const char *>(buf.data());
    size_t n = buf.size();
    return rocksdb::Slice{ptr, n};
  }

  inline BufferView make_span(const rocksdb::Slice &s) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const uint8_t *>(s.data()), s.size()};
  }

  inline common::Buffer make_buffer(const rocksdb::Slice &s) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *ptr = reinterpret_cast<const uint8_t *>(s.data());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return {ptr, ptr + s.size()};
  }
}  // namespace kagome::storage
