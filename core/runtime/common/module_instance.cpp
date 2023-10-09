/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/module_instance.hpp"

#include "common/int_serialization.hpp"
#include "runtime/memory_provider.hpp"
#include "runtime/trie_storage_provider.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::runtime, ModuleInstance::Error, e) {
  using E = kagome::runtime::ModuleInstance::Error;

  switch (e) {
    case E::ABSENT_HEAP_BASE:
      return "Failed to extract heap base from a module";
    case E::HEAP_BASE_TOO_LOW:
      return "Heap base too low";
  }
  return "Unknown ModuleInstance error";
}

namespace kagome::runtime {
  using namespace kagome::common::literals;

  outcome::result<void> ModuleInstance::resetMemory(
      const MemoryLimits &limits) {
    static auto log = log::createLogger("RuntimeEnvironmentFactory", "runtime");

    OUTCOME_TRY(opt_heap_base, getGlobal("__heap_base"));
    if (not opt_heap_base) {
      log->error(
          "__heap_base global variable is not found in a runtime module");
      return ModuleInstance::Error::ABSENT_HEAP_BASE;
    }
    int32_t heap_base = boost::get<int32_t>(*opt_heap_base);
    BOOST_ASSERT(heap_base > 0);
    auto &memory_provider = getEnvironment().memory_provider;
    OUTCOME_TRY(const_cast<MemoryProvider &>(*memory_provider)
                    .resetMemory(MemoryConfig{static_cast<WasmSize>(heap_base),
                                              limits}));
    auto &memory = memory_provider->getCurrentMemory()->get();

    static auto heappages_key = ":heappages"_buf;
    OUTCOME_TRY(heappages,
                getEnvironment().storage_provider->getCurrentBatch()->tryGet(
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
    forDataSegment([&](ModuleInstance::SegmentOffset offset,
                       ModuleInstance::SegmentData segment) {
      max_data_segment_end =
          std::max(max_data_segment_end, offset + segment.size());
    });
    if (gsl::narrow<size_t>(heap_base) < max_data_segment_end) {
      return ModuleInstance::Error::HEAP_BASE_TOO_LOW;
    }

    memory.resize(heap_base);

    forDataSegment([&](auto offset, auto segment) {
      memory.storeBuffer(offset, segment);
    });

    return outcome::success();
  }
}  // namespace kagome::runtime
