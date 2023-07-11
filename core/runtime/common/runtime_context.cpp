/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/runtime_context.hpp"

#include "log/profiling_logger.hpp"
#include "runtime/common/uncompress_code_if_needed.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/trie_storage_provider.hpp"
#include "runtime/instance_environment.hpp"
#include "runtime/module.hpp"
#include "runtime/module_factory.hpp"
#include "storage/trie/polkadot_trie/trie_error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime,
                            RuntimeContext::Error,
                            e) {
  using E = kagome::runtime::RuntimeContext::Error;

  switch (e) {
    case E::ABSENT_HEAP_BASE:
      return "Failed to extract heap base from a module";
    case E::HEAP_BASE_TOO_LOW:
      return "Heap base too low";
  }
  return "Unknown runtime environment construction error";
}

namespace kagome::runtime {
  using namespace kagome::common::literals;

  RuntimeContext::RuntimeContext(
      std::shared_ptr<ModuleInstance> module_instance)
      : module_instance{std::move(module_instance)} {
    BOOST_ASSERT(this->module_instance);
  }


  outcome::result<void> resetMemory(const ModuleInstance &instance) {
    static auto log = log::createLogger("RuntimeEnvironmentFactory", "runtime");

    OUTCOME_TRY(opt_heap_base, instance.getGlobal("__heap_base"));
    if (not opt_heap_base) {
      log->error(
          "__heap_base global variable is not found in a runtime module");
      return RuntimeContext::Error::ABSENT_HEAP_BASE;
    }
    int32_t heap_base = boost::get<int32_t>(*opt_heap_base);

    auto &memory_provider = instance.getEnvironment().memory_provider;
    OUTCOME_TRY(
        const_cast<MemoryProvider &>(*memory_provider).resetMemory(heap_base));
    auto &memory = memory_provider->getCurrentMemory()->get();

    static auto heappages_key = ":heappages"_buf;
    OUTCOME_TRY(
        heappages,
        instance.getEnvironment().storage_provider->getCurrentBatch()->tryGet(
            heappages_key));
    if (heappages) {
      if (sizeof(uint64_t) != heappages->size()) {
        log->error(
            "Unable to read :heappages value. Type size mismatch. "
            "Required {} bytes, but {} available",
            sizeof(uint64_t),
            heappages->size());
      } else {
        uint64_t pages = common::le_bytes_to_uint64(heappages->view());
        memory.resize(pages * kMemoryPageSize);
        log->trace(
            "Creating wasm module with non-default :heappages value set to {}",
            pages);
      }
    }

    size_t max_data_segment_end = 0;
    instance.forDataSegment([&](ModuleInstance::SegmentOffset offset,
                                ModuleInstance::SegmentData segment) {
      max_data_segment_end =
          std::max(max_data_segment_end, offset + segment.size());
    });
    if (gsl::narrow<size_t>(heap_base) < max_data_segment_end) {
      return RuntimeContext::Error::HEAP_BASE_TOO_LOW;
    }

    memory.resize(heap_base);

    instance.forDataSegment([&](auto offset, auto segment) {
      memory.storeBuffer(offset, segment);
    });

    return outcome::success();
  }

  outcome::result<RuntimeContext> RuntimeContext::fromCode(
      const runtime::ModuleFactory &module_factory,
      common::BufferView code_zstd) {
    common::Buffer code;
    OUTCOME_TRY(runtime::uncompressCodeIfNeeded(code_zstd, code));
    OUTCOME_TRY(runtime_module, module_factory.make(code));
    OUTCOME_TRY(instance, runtime_module->instantiate());
    runtime::RuntimeContext ctx{
        instance,
    };
    instance->getEnvironment()
        .storage_provider->setToEphemeralAt(storage::trie::kEmptyRootHash)
        .value();
    OUTCOME_TRY(resetMemory(*instance));
    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContext::persistent(
      std::shared_ptr<ModuleInstance> instance,
      const storage::trie::RootHash &state,
      std::optional<std::shared_ptr<storage::changes_trie::ChangesTracker>>
          changes_tracker_opt) {
    runtime::RuntimeContext ctx{
        instance,
    };
    OUTCOME_TRY(instance->getEnvironment().storage_provider->setToPersistentAt(
        state, changes_tracker_opt));
    OUTCOME_TRY(resetMemory(*instance));
    return ctx;
  }

  outcome::result<RuntimeContext> RuntimeContext::ephemeral(
      std::shared_ptr<ModuleInstance> instance,
      const storage::trie::RootHash &state) {
    runtime::RuntimeContext ctx{
        instance,
    };
    OUTCOME_TRY(
        instance->getEnvironment().storage_provider->setToEphemeralAt(state));
    OUTCOME_TRY(resetMemory(*instance));
    return ctx;
  }

}  // namespace kagome::runtime
