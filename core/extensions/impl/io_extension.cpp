/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/io_extension.hpp"
#include <runtime/wasm_result.hpp>

namespace kagome::extensions {
  IOExtension::IOExtension(std::shared_ptr<runtime::WasmMemory> memory)
      : memory_(std::move(memory)),
        logger_{common::createLogger(kDefaultLoggerTag)} {
    BOOST_ASSERT_MSG(memory_ != nullptr, "memory is nullptr");
  }

  void IOExtension::ext_print_hex(runtime::WasmPointer data,
                                  runtime::WasmSize length) {
    const auto &buf = memory_->loadN(data, length);
    logger_->info("hex value: {}", buf.toHex());
  }

  void IOExtension::ext_logging_log_version_1(
      runtime::WasmEnum level,
      runtime::WasmSpan target,
      runtime::WasmSpan message) {
    using runtime::WasmResult;
    using runtime::WasmLogLevel;

    auto read_str_from_position = [&] (WasmResult location) {
      return memory_->loadStr(location.address, location.length);
    };

    const auto target_str = read_str_from_position(WasmResult(target));
    const auto message_str = read_str_from_position(WasmResult(message));

    switch (level) {
      case WasmLogLevel::WasmLL_Error:
        logger_->error(
            "target: {}\n\tmessage: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Warn:
        logger_->warn(
            "target: {}\n\tmessage: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Info:
        logger_->info(
            "target: {}\n\tmessage: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Debug:
        logger_->debug(
            "target: {}\n\tmessage: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Trace:
        logger_->trace(
            "target: {}\n\tmessage: {}", target_str, message_str);
        break;
      default: {
        assert(false);
        logger_->error(
            "Message with incorrect log level. Target: {}\n\tmessage: {}", target_str, message_str);
      } break;
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
}  // namespace kagome::extensions
