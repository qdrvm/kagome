/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "extensions/impl/misc_extension.hpp"

#include "runtime/common/const_wasm_provider.hpp"
#include "runtime/core_factory.hpp"
#include "runtime/wasm_memory.hpp"
#include "scale/scale.hpp"

namespace kagome::extensions {

  MiscExtension::MiscExtension(
      uint64_t chain_id,
      std::shared_ptr<runtime::WasmMemory> memory,
      CoreFactoryMethod core_factory_method)
      : core_factory_method_{std::move(core_factory_method)},
        memory_{std::move(memory)},
        logger_{common::createLogger("MiscExtension")},
        chain_id_{chain_id} {
    BOOST_ASSERT(memory_);
  }

  uint64_t MiscExtension::ext_chain_id() const {
    return chain_id_;
  }

  runtime::WasmResult MiscExtension::ext_misc_runtime_version_version_1(
      runtime::WasmSpan data) const {
    auto [ptr, len] = runtime::splitSpan(data);
    auto code = memory_->loadN(ptr, len);
    auto wasm_provider =
        std::make_shared<runtime::ConstWasmProvider>(std::move(code));
    auto core = core_factory_method_(wasm_provider);
    auto version_res = core->version(boost::none);

    static const auto error_res =
        scale::encode<boost::optional<primitives::Version>>(boost::none)
            .value();

    if (version_res.has_value()) {
      auto enc_version_res =
          scale::encode(boost::make_optional(version_res.value()));
      if (enc_version_res.has_error()) {
        logger_->error(
            "Error encoding ext_misc_runtime_version_version_1 result: {}",
            enc_version_res.error().message());
        return runtime::WasmResult{memory_->storeBuffer(error_res)};
      }
      auto res_span = memory_->storeBuffer(enc_version_res.value());
      return runtime::WasmResult{res_span};
    }
    logger_->error("Error inside Core_version: {}",
                   version_res.error().message());
    return runtime::WasmResult{memory_->storeBuffer(error_res)};
  }

  void MiscExtension::ext_misc_print_hex_version_1(
      runtime::WasmSpan data) const {
    auto [ptr, len] = runtime::splitSpan(data);
    auto buf = memory_->loadN(ptr, len);
    logger_->info("hex: {}", buf.toHex());
  }

  void MiscExtension::ext_misc_print_num_version_1(
      uint64_t value) const {
    logger_->info("num: {}", value);
  }

  void MiscExtension::ext_misc_print_utf8_version_1(
      runtime::WasmSpan data) const {
    auto [ptr, len] = runtime::splitSpan(data);
    auto buf = memory_->loadN(ptr, len);
    logger_->info("utf8: {}", buf.toString());
  }

}  // namespace kagome::extensions
