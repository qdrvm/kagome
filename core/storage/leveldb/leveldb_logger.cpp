/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/leveldb/leveldb_logger.hpp"

namespace kagome::storage {

  void LevelDB::Logger::Logv(const char *format, va_list ap) {
    // copied from Bitcoin: https://github.com/bitcoin/bitcoin/pull/9999/files
    char buffer[500];
    for (int iter = 0; iter < 2; iter++) {
      char *base;
      int bufsize;
      if (iter == 0) {
        bufsize = sizeof(buffer);
        base = buffer;
      } else {
        bufsize = 30000;
        base = new char[bufsize];
      }
      char *p = base;
      char *limit = base + bufsize;

      // Print the message
      if (p < limit) {
        va_list backup_ap;
        va_copy(backup_ap, ap);
        // Do not use vsnprintf elsewhere in bitcoin source code, see above.
        p += vsnprintf(p, limit - p, format, backup_ap);
        va_end(backup_ap);
      }

      // Truncate to available space if necessary
      if (p >= limit) {
        if (iter == 0) {
          continue;  // Try again with larger buffer
        } else {
          p = limit - 1;
        }
      }

      // Add newline if necessary
      if (p == base || p[-1] != '\n') {
        *p++ = '\n';
      }

      assert(p <= limit);
      base[std::min(bufsize - 1, (int)(p - base))] = '\0';
      logger_->debug(base);
      if (base != buffer) {
        delete[] base;
      }
      break;
    }
  }

  LevelDB::Logger::Logger(common::Logger logger) : logger_(std::move(logger)) {}
}  // namespace kagome::storage
