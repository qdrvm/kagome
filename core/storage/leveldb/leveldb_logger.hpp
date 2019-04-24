/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LEVELDB_LOGGER_HPP
#define KAGOME_LEVELDB_LOGGER_HPP

#include "storage/leveldb/leveldb.hpp"

#include <leveldb/env.h>
#include "common/logger.hpp"

namespace kagome::storage {

  /**
   * @brief Logger, which can be used to log LevelDB internal events.
   * @see leveldb::Options#info_log
   */
  class LevelDB::Logger : public leveldb::Logger {
   public:
    explicit Logger(common::Logger logger);

    void Logv(const char *format, va_list ap) override;

   private:
    common::Logger logger_;
  };

}  // namespace kagome::storage

#endif  // KAGOME_LEVELDB_LOGGER_HPP
