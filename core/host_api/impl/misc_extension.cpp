/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_api/impl/misc_extension.hpp"

#include "log/trace_macros.hpp"
#include "primitives/version.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/core_api_factory.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_api/core.hpp"
#include "scale/scale.hpp"

namespace kagome::host_api {

  MiscExtension::MiscExtension(
      uint64_t chain_id,
      std::shared_ptr<const crypto::Hasher> hasher,
      std::shared_ptr<const runtime::MemoryProvider> memory_provider,
      std::shared_ptr<const runtime::CoreApiFactory> core_factory)
      : hasher_{std::move(hasher)},
        memory_provider_{std::move(memory_provider)},
        core_factory_{std::move(core_factory)},
        logger_{log::createLogger("MiscExtension", "misc_extension")} {
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(core_factory_);
    BOOST_ASSERT(memory_provider_);
  }

  runtime::WasmSpan MiscExtension::ext_misc_runtime_version_version_1(
      runtime::WasmSpan data) const {
    auto [ptr, len] = runtime::splitSpan(data);
    auto &memory = memory_provider_->getCurrentMemory()->get();

    auto code = memory.loadN(ptr, len);
    common::Buffer uncompressed_code;
    auto uncompress_res =
        runtime::uncompressCodeIfNeeded(code, uncompressed_code);

    static const auto kErrorRes =
        scale::encode<std::optional<primitives::Version>>(std::nullopt).value();

    if (uncompress_res.has_error()) {
      SL_ERROR(logger_, "Error decompressing code: {}", uncompress_res.error());
      return memory.storeBuffer(kErrorRes);
    }

    auto core_api =
        core_factory_->make(hasher_, uncompressed_code.asVector()).value();
    auto version_res = core_api->version();
    SL_TRACE_FUNC_CALL(logger_, version_res.has_value(), data);

    if (version_res.has_value()) {
      auto enc_version_res = scale::encode(
          std::make_optional(scale::encode(version_res.value()).value()));
      if (enc_version_res.has_error()) {
        SL_ERROR(logger_,
                 "Error encoding ext_misc_runtime_version_version_1 result: {}",
                 enc_version_res.error());
        return memory.storeBuffer(kErrorRes);
      }
      auto res_span = memory.storeBuffer(enc_version_res.value());
      return res_span;
    }
    SL_ERROR(logger_, "Error inside Core_version: {}", version_res.error());
    return memory.storeBuffer(kErrorRes);
  }

  void MiscExtension::ext_misc_print_hex_version_1(
      runtime::WasmSpan data) const {
    auto [ptr, len] = runtime::splitSpan(data);
    auto buf = memory_provider_->getCurrentMemory()->get().loadN(ptr, len);
    logger_->info("hex: {}", buf.toHex());
  }

  void MiscExtension::ext_misc_print_num_version_1(int64_t value) const {
    logger_->info("num: {}", value);
  }

  void MiscExtension::ext_misc_print_utf8_version_1(
      runtime::WasmSpan data) const {
    auto [ptr, len] = runtime::splitSpan(data);
    auto buf = memory_provider_->getCurrentMemory()->get().loadN(ptr, len);
    logger_->info("utf8: {}", buf.toStringView());
  }

}  // namespace kagome::host_api
