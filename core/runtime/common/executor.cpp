/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/executor.hpp"

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

  Executor::Executor(
      std::shared_ptr<RuntimeContextFactory> ctx_factory,
      std::optional<std::shared_ptr<RuntimePropertiesCache>> cache)
      : cache_{cache}, ctx_factory_{ctx_factory} {}

}  // namespace kagome::runtime
