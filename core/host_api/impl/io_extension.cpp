/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/io_extension.hpp"

#include <boost/assert.hpp>

#include "runtime/wavm/impl/memory.hpp"
#include "runtime/wasm_result.hpp"

namespace kagome::host_api {
  IOExtension::IOExtension(std::shared_ptr<runtime::wavm::Memory> memory)
      : memory_(std::move(memory)),
        logger_{log::createLogger("IoExtension", "host_api")} {
    BOOST_ASSERT_MSG(memory_ != nullptr, "memory is nullptr");
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
        logger_->error("target: {}\n\tmessage: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Warn:
        logger_->warn("target: {}\n\tmessage: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Info:
        logger_->info("target: {}\n\tmessage: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Debug:
        SL_DEBUG(logger_, "target: {}\n\tmessage: {}", target_str, message_str);
        break;
      case WasmLogLevel::WasmLL_Trace:
        SL_TRACE(logger_, "target: {}\n\tmessage: {}", target_str, message_str);
        break;
      default: {
        BOOST_UNREACHABLE_RETURN();
        logger_->error(
            "Message with incorrect log level. Target: {}\n\tmessage: {}",
            target_str,
            message_str);
      }
    }
  }

}  // namespace kagome::host_api
