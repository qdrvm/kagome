/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/module_instance.hpp"

#include <cstring>

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
    case E::INVALID_CALL_RESULT:
      return "The size of the buffer returned by the runtime does not match "
             "the size of the requested return type";
  }
  return "Unknown ModuleInstance error";
}

namespace kagome::runtime {
  outcome::result<void> ModuleInstance::resetMemory() {
    static auto log = log::createLogger("RuntimeEnvironmentFactory", "runtime");

    OUTCOME_TRY(opt_heap_base, getGlobal("__heap_base"));
    if (not opt_heap_base) {
      log->error(
          "__heap_base global variable is not found in a runtime module");
      return ModuleInstance::Error::ABSENT_HEAP_BASE;
    }
    uint32_t heap_base = boost::get<int32_t>(*opt_heap_base);
    auto &memory_provider = getEnvironment().memory_provider;
    OUTCOME_TRY(const_cast<MemoryProvider &>(*memory_provider)
                    .resetMemory(MemoryConfig{heap_base}));
    auto &memory = memory_provider->getCurrentMemory()->get();

    size_t max_data_segment_end = 0;
    size_t segments_num = 0;
    forDataSegment([&](ModuleInstance::SegmentOffset offset,
                       ModuleInstance::SegmentData segment) {
      max_data_segment_end =
          std::max(max_data_segment_end, offset + segment.size());
      segments_num++;
      SL_TRACE(log,
               "Data segment {} at offset {}",
               common::BufferView{segment},
               offset);
    });
    if (static_cast<size_t>(heap_base) < max_data_segment_end) {
      SL_WARN(
          log,
          "__heap_base too low, allocations will overwrite wasm data segments");
    }

    forDataSegment([&](auto offset, auto segment) {
      memory.storeBuffer(offset, segment);
    });

    return outcome::success();
  }

}  // namespace kagome::runtime
