/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/io_extension.hpp"

#include <boost/assert.hpp>

#include "runtime/wasm_memory.hpp"
#include "runtime/wasm_result.hpp"

namespace kagome::host_api {
  IOExtension::IOExtension(std::shared_ptr<runtime::WasmMemory> memory)
      : memory_(std::move(memory)),
        logger_{log::createLogger("IoExtention", "extentions")} {
    BOOST_ASSERT_MSG(memory_ != nullptr, "memory is nullptr");
  }

  void IOExtension::ext_print_hex(runtime::WasmPointer data,
                                  runtime::WasmSize length) {
    const auto &buf = memory_->loadN(data, length);
    logger_->info("hex value: {}", buf.toHex());
  }

  void IOExtension::ext_logging_log_version_1(runtime::WasmEnum level,
                                              runtime::WasmSpan target,
                                              runtime::WasmSpan message) {
    using runtime::WasmLogLevel;
    using runtime::WasmResult;

    auto read_str_from_position = [&](WasmResult location) {
      return memory_->loadStr(location.address, location.length);
    };

    const auto target_str = read_str_from_position(WasmResult(target));
    const auto message_str = read_str_from_position(WasmResult(message));

    switch (level) {
      case WasmLogLevel::WasmLL_Error:
        logger_->error("target: {}, message: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Warn:
        logger_->warn("target: {}, message: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Info:
        logger_->info("target: {}, message: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Debug:
        SL_DEBUG(logger_, "target: {}, message: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Trace:
        SL_TRACE(logger_, "target: {}, message: {}", target_str, message_str);
        break;
      default: {
        assert(false);
        logger_->error(
            "Message with incorrect log level. Target: {}, message: {}",
            target_str,
            message_str);
      }
    }
  }

  runtime::WasmEnum IOExtension::ext_logging_max_level_version_1() {
    using runtime::WasmLogLevel;
    using soralog::Level;

    switch (logger_->level()) {
      case Level::OFF:
      case Level::CRITICAL:
      case Level::ERROR:
        return WasmLogLevel::WasmLL_Error;
      case Level::WARN:
        return WasmLogLevel::WasmLL_Warn;
      case Level::INFO:
      case Level::VERBOSE:
        return WasmLogLevel::WasmLL_Info;
      case Level::DEBUG:
        return WasmLogLevel::WasmLL_Debug;
      case Level::TRACE:
        return WasmLogLevel::WasmLL_Trace;
      default:
        return WasmLogLevel::WasmLL_Error;
    }
  }

  void IOExtension::ext_print_num(uint64_t value) {
    logger_->info("number value: {}", value);
  }

  void IOExtension::ext_print_utf8(runtime::WasmPointer utf8_data,
                                   runtime::WasmSize utf8_length) {
    const auto data = memory_->loadStr(utf8_data, utf8_length);
    logger_->info("string value: {}", data);
  }
}  // namespace kagome::host_api
