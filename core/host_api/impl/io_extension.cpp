/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/io_extension.hpp"

#include <boost/assert.hpp>

#include "runtime/memory.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/ptr_size.hpp"

namespace kagome::host_api {
  IOExtension::IOExtension(
      std::shared_ptr<const runtime::MemoryProvider> memory_provider)
      : memory_provider_(std::move(memory_provider)),
        logger_{log::createLogger("IoExtension", "io_extension")} {
    BOOST_ASSERT_MSG(memory_provider_ != nullptr, "memory provider is nullptr");
  }

  soralog::Level mapLevel(runtime::WasmLogLevel wasm_level) {
    using WasmLevel = runtime::WasmLogLevel;
    using SlLevel = soralog::Level;
    switch (wasm_level) {
      case WasmLevel::Error:
        return SlLevel::ERROR;
      case WasmLevel::Warn:
        return SlLevel::WARN;
      case WasmLevel::Info:
        return SlLevel::INFO;
      case WasmLevel::Debug:
        return SlLevel::DEBUG;
      case WasmLevel::Trace:
        return SlLevel::TRACE;
    }
  }

  void IOExtension::ext_logging_log_version_1(runtime::WasmEnum wasm_level,
                                              runtime::WasmSpan target,
                                              runtime::WasmSpan message) {
    using runtime::WasmLogLevel;

    auto read_str_from_position = [&](runtime::PtrSize location) {
      return memory_provider_->getCurrentMemory()->get().loadStr(location.ptr,
                                                                 location.size);
    };

    const auto target_str = read_str_from_position(runtime::PtrSize(target));
    const auto message_str = read_str_from_position(runtime::PtrSize(message));

    auto level = std::max(mapLevel(static_cast<WasmLogLevel>(wasm_level)),
                          soralog::Level::VERBOSE);
    logger_->log(level, "{}: {}", target_str, message_str);
  }

  runtime::WasmEnum IOExtension::ext_logging_max_level_version_1() {
    using runtime::WasmLogLevel;
    using soralog::Level;

    switch (logger_->level()) {
      case Level::OFF:
      case Level::CRITICAL:
      case Level::ERROR:
        return static_cast<runtime::WasmEnum>(WasmLogLevel::Error);
      case Level::WARN:
        return static_cast<runtime::WasmEnum>(WasmLogLevel::Warn);
      case Level::INFO:
      case Level::VERBOSE:
        return static_cast<runtime::WasmEnum>(WasmLogLevel::Info);
      case Level::DEBUG:
        return static_cast<runtime::WasmEnum>(WasmLogLevel::Debug);
      case Level::TRACE:
        return static_cast<runtime::WasmEnum>(WasmLogLevel::Trace);
      default:
        return static_cast<runtime::WasmEnum>(WasmLogLevel::Error);
    }
  }

}  // namespace kagome::host_api
