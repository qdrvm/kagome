/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_IMPL_HPP
#define KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_IMPL_HPP

#include "runtime/executor.hpp"

#include <optional>

#include "blockchain/block_header_repository.hpp"
#include "common/buffer.hpp"
#include "host_api/host_api.hpp"
#include "log/profiling_logger.hpp"
#include "outcome/outcome.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/module_repository.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "scale/scale.hpp"
#include "storage/trie/trie_batches.hpp"

namespace kagome::runtime {

  Executor::Executor(std::shared_ptr<RuntimeContextFactory> ctx_factory,
                     std::shared_ptr<RuntimePropertiesCache> cache)
      : ctx_factory_{ctx_factory}, cache_{cache} {}

  outcome::result<common::Buffer> Executor::callWithCtx(
      RuntimeContext &ctx, std::string_view name, const Buffer &encoded_args) {
    KAGOME_PROFILE_START(call_execution)
    OUTCOME_TRY(result,
                ctx.module_instance->callExportFunction(name, encoded_args));
    auto memory = ctx.module_instance->getEnvironment()
                      .memory_provider->getCurrentMemory();
    BOOST_ASSERT(memory.has_value());
    OUTCOME_TRY(ctx.module_instance->resetEnvironment());
    return memory->get().loadN(result.ptr, result.size);
  }

}  // namespace kagome::runtime

#undef likely

#endif  // KAGOME_CORE_RUNTIME_COMMON_EXECUTOR_HPP
