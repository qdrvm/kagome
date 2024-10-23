/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wabt/instrument.hpp"

#include <algorithm>

#include "runtime/wabt/stack_limiter.hpp"
#include "runtime/wabt/util.hpp"

namespace kagome::runtime {
  WabtOutcome<void> convertMemoryImportIntoExport(wabt::Module &module) {
    for (auto it = module.fields.begin(); it != module.fields.end(); ++it) {
      auto import = dynamic_cast<const wabt::ImportModuleField *>(&*it);
      if (not import) {
        continue;
      }
      auto memory = dynamic_cast<wabt::MemoryImport *>(import->import.get());
      if (not memory) {
        continue;
      }
      // NOLINTNEXTLINE(modernize-use-ranges,boost-use-ranges)
      if (std::any_of(module.fields.begin(),
                      module.fields.end(),
                      [&](const wabt::ModuleField &field) {
                        return field.type() == wabt::ModuleFieldType::Memory;
                      })) {
        return WabtError{"unexpected MemoryModuleField"};
      }
      auto import_it = std::ranges::find(module.imports, import->import.get());
      if (import_it == module.imports.end()) {
        return WabtError{"inconsistent Module.imports"};
      }
      auto memory_it = std::ranges::find(module.memories, &memory->memory);
      if (memory_it == module.memories.end()) {
        return WabtError{"inconsistent Module.memories"};
      }
      auto memory2 = std::make_unique<wabt::MemoryModuleField>();
      memory2->memory.page_limits = memory->memory.page_limits;
      auto export_ = std::make_unique<wabt::ExportModuleField>();
      export_->export_ = {
          .name = import->import->field_name,
          .kind = wabt::ExternalKind::Memory,
          .var = wabt::Var{0, {}},
      };
      module.imports.erase(import_it);
      module.memories.erase(memory_it);
      module.fields.erase(it);
      --module.num_memory_imports;
      module.AppendField(std::move(memory2));
      module.AppendField(std::move(export_));
      break;
    }
    return outcome::success();
  }

  WabtOutcome<void> setupMemoryAccordingToHeapAllocStrategy(
      wabt::Module &module, const HeapAllocStrategy &config) {
    for (auto &field : module.fields) {
      auto memory = dynamic_cast<wabt::MemoryModuleField *>(&field);
      if (not memory) {
        continue;
      }
      auto &limit = memory->memory.page_limits;
      auto init = limit.initial;
      if (auto v = boost::get<HeapAllocStrategyDynamic>(&config)) {
        if (auto &max = v->maximum_pages) {
          limit = {std::min<uint64_t>(init, *max), *max};
        } else {
          limit = wabt::Limits{init};
        }
      } else {
        auto max =
            init + boost::get<HeapAllocStrategyStatic>(config).extra_pages;
        limit = {max, max};
      }
    }
    return outcome::success();
  }

  WabtOutcome<common::Buffer> instrumentCodeForCompilation(
      common::BufferView code, const RuntimeContext::ContextParams &config) {
    OUTCOME_TRY(module, wabtDecode(code, config));
    const auto &memory_config = config.memory_limits;
    if (memory_config.max_stack_values_num) {
      OUTCOME_TRY(instrumentWithStackLimiter(
          module, *memory_config.max_stack_values_num));
    }
    OUTCOME_TRY(convertMemoryImportIntoExport(module));
    OUTCOME_TRY(setupMemoryAccordingToHeapAllocStrategy(
        module, memory_config.heap_alloc_strategy));
    OUTCOME_TRY(wabtValidate(module));
    return wabtEncode(module);
  }

  WabtOutcome<common::Buffer> WasmInstrumenter::instrument(
      common::BufferView code, const RuntimeContext::ContextParams &config) const {
    return instrumentCodeForCompilation(code, config);
  }
}  // namespace kagome::runtime
