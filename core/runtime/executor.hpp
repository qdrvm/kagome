/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "outcome/outcome.hpp"
#include "primitives/common.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/runtime_properties_cache.hpp"

namespace kagome::runtime {

  class Executor {
   public:
    Executor(std::shared_ptr<RuntimeContextFactory> ctx_factory,
             std::optional<std::shared_ptr<RuntimePropertiesCache>> cache);

    const RuntimeContextFactory &ctx() const {
      return *ctx_factory_;
    }

    template <typename Res, typename... Args>
    outcome::result<Res> call(RuntimeContext &ctx,
                              std::string_view name,
                              const Args &...args) {
      auto code_hash = ctx.module_instance->getCodeHash();
      auto call = [&]() {
        return ctx.module_instance->template callAndDecodeExportFunction<Res>(
            ctx, name, args...);
      };
      if (!cache_) {
        return call();
      }
      auto &cache = *cache_;
      if constexpr (std::is_same_v<Res, primitives::Version>) {
        [[likely]] if (name == "Core_version") {
          return cache->getVersion(code_hash, call);
        }
      }
      if constexpr (std::is_same_v<Res, primitives::OpaqueMetadata>) {
        [[likely]] if (name == "Metadata_metadata") {
          return cache->getMetadata(code_hash, call);
        }
      }
      return call();
    }

    std::optional<std::shared_ptr<RuntimePropertiesCache>> cache_;
    std::shared_ptr<const RuntimeContextFactory> ctx_factory_;
  };

}  // namespace kagome::runtime
