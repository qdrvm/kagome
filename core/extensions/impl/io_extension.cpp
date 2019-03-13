/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"
#include "extensions/impl/io_extension.hpp"

namespace kagome::extensions {
  IOExtension::IOExtension()
      : logger_{common::logger::createLogger("WASM Runtime")} {}

  void IOExtension::ext_print_hex(const uint8_t *data, uint32_t length) {
    logger_->info("hex value: {}", common::hex_lower(data, length));
  }

  void IOExtension::ext_print_num(uint64_t value) {
    logger_->info("number value: {}", value);
  }

  void IOExtension::ext_print_utf8(const uint8_t *utf8_data,
                                   uint32_t utf8_length) {
    auto *str_begin = utf8_data;
    std::advance(utf8_data, utf8_length);
    logger_->info("string value: {}", std::string{str_begin, utf8_data});
  }
}  // namespace kagome::extensions
