/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "extensions/impl/io_extension.hpp"

namespace kagome::extensions {
  IOExtension::IOExtension(std::shared_ptr<runtime::WasmMemory> memory)
      : memory_(std::move(memory)),
        logger_{common::createLogger(kDefaultLoggerTag)} {
    BOOST_ASSERT_MSG(memory_ != nullptr, "memory is nullptr");
  }

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
