/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/misc_extension.hpp"

#include "primitives/version.hpp"
#include "runtime/core.hpp"
#include "runtime/core_api_provider.hpp"
#include "runtime/wasm_memory.hpp"
#include "runtime/wavm/executor.hpp"
#include "runtime/wavm/impl/intrinsic_resolver_impl.hpp"
#include "runtime/wavm/module_repository.hpp"
#include "scale/scale.hpp"

#include "runtime/wavm/impl/crutch.hpp"

namespace kagome::host_api {

  MiscExtension::MiscExtension(
      uint64_t chain_id,
      std::shared_ptr<runtime::Memory> memory,
      std::shared_ptr<const runtime::CoreApiProvider> core_provider)
      : memory_{std::move(memory)},
        core_provider_{std::move(core_provider)},
        logger_{log::createLogger("MiscExtension", "host_api")},
        chain_id_{chain_id} {
    BOOST_ASSERT(core_provider_);
    BOOST_ASSERT(memory_);
  }

  runtime::WasmSpan MiscExtension::ext_misc_runtime_version_version_1(
      runtime::WasmSpan data) const {
    auto [ptr, len] = runtime::splitSpan(data);

    auto core_api =
        core_provider_->makeCoreApi(memory_->loadN(ptr, len).asVector());
    auto version_res = core_api->version(boost::none);
    runtime::wavm::global_host_apis.pop();

    static const auto kErrorRes =
        scale::encode<boost::optional<primitives::Version>>(boost::none)
            .value();

    if (version_res.has_value()) {
      auto enc_version_res = scale::encode(
          boost::make_optional(scale::encode(version_res.value()).value()));
      if (enc_version_res.has_error()) {
        logger_->error(
            "Error encoding ext_misc_runtime_version_version_1 result: {}",
            enc_version_res.error().message());
        return memory_->storeBuffer(kErrorRes);
      }
      auto res_span = memory_->storeBuffer(enc_version_res.value());
      return res_span;
    }
    logger_->error("Error inside Core_version: {}",
                   version_res.error().message());
    return memory_->storeBuffer(kErrorRes);
  }

  void MiscExtension::ext_misc_print_hex_version_1(
      runtime::WasmSpan data) const {
    auto [ptr, len] = runtime::splitSpan(data);
    auto buf = memory_->loadN(ptr, len);
    logger_->info("hex: {}", buf.toHex());
  }

  void MiscExtension::ext_misc_print_num_version_1(uint64_t value) const {
    logger_->info("num: {}", value);
  }

  void MiscExtension::ext_misc_print_utf8_version_1(
      runtime::WasmSpan data) const {
    auto [ptr, len] = runtime::splitSpan(data);
    auto buf = memory_->loadN(ptr, len);
    logger_->info("utf8: {}", buf.toString());
  }

}  // namespace kagome::host_api
