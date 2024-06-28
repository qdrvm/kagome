/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/wabt/instrument.hpp"

#include "log/logger.hpp"
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
      if (std::any_of(module.fields.begin(),
                      module.fields.end(),
                      [&](const wabt::ModuleField &field) {
                        return field.type() == wabt::ModuleFieldType::Memory;
                      })) {
        return WabtError{"unexpected MemoryModuleField"};
      }
      auto import_it = std::find(
          module.imports.begin(), module.imports.end(), import->import.get());
      if (import_it == module.imports.end()) {
        return WabtError{"inconsistent Module.imports"};
      }
      auto memory_it = std::find(
          module.memories.begin(), module.memories.end(), &memory->memory);
      if (memory_it == module.memories.end()) {
        return WabtError{"inconsistent Module.memories"};
      }
      auto memory2 = std::make_unique<wabt::MemoryModuleField>();
      memory2->memory.page_limits = memory->memory.page_limits;
      auto export_ = std::make_unique<wabt::ExportModuleField>();
      export_->export_ = {
          import->import->field_name,
          wabt::ExternalKind::Memory,
          wabt::Var{0, {}},
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
    for (auto it = module.fields.begin(); it != module.fields.end(); ++it) {
      auto memory = dynamic_cast<wabt::MemoryModuleField *>(&*it);
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
      log::createLogger("wabt", "runtime")
          ->info("Memory after patching: {} {}",
                 memory->memory.page_limits.initial,
                 memory->memory.page_limits.max);
    }
    return outcome::success();
  }

  WabtOutcome<common::Buffer> prepareBlobForCompilation(
      common::BufferView code, const MemoryLimits &config) {
    OUTCOME_TRY(module, wabtDecode(code));
    if (config.max_stack_values_num) {
      OUTCOME_TRY(
          instrumentWithStackLimiter(module, *config.max_stack_values_num));
    }
    OUTCOME_TRY(convertMemoryImportIntoExport(module));
    OUTCOME_TRY(setupMemoryAccordingToHeapAllocStrategy(
        module, config.heap_alloc_strategy));
    OUTCOME_TRY(wabtValidate(module));
    return wabtEncode(module);
  }

  WabtOutcome<common::Buffer> InstrumentWasm::instrument(
      common::BufferView code, const MemoryLimits &config) const {
    return prepareBlobForCompilation(code, config);
  }
}  // namespace kagome::runtime
