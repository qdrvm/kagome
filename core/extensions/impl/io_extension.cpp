/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/io_extension.hpp"

namespace kagome::extensions {
  IOExtension::IOExtension(std::shared_ptr<runtime::WasmMemory> memory)
      : memory_(std::move(memory)),
        logger_{common::createLogger(kDefaultLoggerTag)} {}

  void IOExtension::ext_print_hex(runtime::WasmPointer data,
                                  runtime::SizeType length) {
    const auto &buf = memory_->loadN(data, length);
    logger_->info("hex value: {}", buf.toHex());
  }

  void IOExtension::ext_print_num(uint64_t value) {
    logger_->info("number value: {}", value);
  }

  void IOExtension::ext_print_utf8(runtime::WasmPointer utf8_data,
                                   runtime::SizeType utf8_length) {
    const auto &buf = memory_->loadN(utf8_data, utf8_length);
    logger_->info("string value: {}", std::string{buf.begin(), buf.end()});
  }
}  // namespace kagome::extensions
