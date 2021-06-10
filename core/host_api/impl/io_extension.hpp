/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IO_EXTENSION_HPP
#define KAGOME_IO_EXTENSION_HPP

#include <cstdint>

#include "log/logger.hpp"
#include "runtime/types.hpp"

namespace kagome::runtime {
  class MemoryProvider;
}

namespace kagome::host_api {

  /**
   * Implements extension functions related to IO
   */
  class IOExtension {
   public:
    explicit IOExtension(
        std::shared_ptr<const runtime::MemoryProvider> memory_provider);

    /**
     * @see Extension::ext_logging_log_version_1
     */
    void ext_logging_log_version_1(runtime::WasmEnum level,
                                   runtime::WasmSpan target,
                                   runtime::WasmSpan message);

    /**
     * @see Extension::ext_logging_max_level_version_1
     */
    runtime::WasmEnum ext_logging_max_level_version_1();

   private:
    std::shared_ptr<const runtime::MemoryProvider> memory_provider_;
    log::Logger logger_;
  };
}  // namespace kagome::host_api

#endif  // KAGOME_IO_EXTENSION_HPP
