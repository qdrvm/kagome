/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "runtime/wabt/error.hpp"

namespace wabt {
  struct Module;
}  // namespace wabt

namespace kagome::runtime {
  struct MemoryLimits;

  WabtOutcome<void> convertMemoryImportIntoExport(wabt::Module &module);

  WabtOutcome<void> setupMemoryAccordingToHeapAllocStrategy(
      wabt::Module &module, const HeapAllocStrategy &config);

  /**
   * Instrument wasm code:
   * - add stack limiting
   * - convert imported memory (if any) to exported memory
   * - set memory limit
   * https://github.com/paritytech/polkadot-sdk/blob/11831df8e709061e9c6b3292facb5d7d9709f151/substrate/client/executor/wasmtime/src/runtime.rs#L651
   */
  WabtOutcome<common::Buffer> prepareBlobForCompilation(
      common::BufferView code, const MemoryLimits &config);

  class InstrumentWasm {
   public:
    virtual ~InstrumentWasm() = default;

    virtual WabtOutcome<common::Buffer> instrument(
        common::BufferView code, const MemoryLimits &config) const;
  };
}  // namespace kagome::runtime
