#pragma once

#include "runtime/runtime_context.hpp"

namespace kagome::parachain {
  static constexpr uint64_t DEFAULT_BACKING_EXECUTION_TIMEOUT = 2000;
  static constexpr uint64_t DEFAULT_APPROVAL_EXECUTION_TIMEOUT = 12000;
  static constexpr bool DEFAULT_WASM_EXT_BULK_MEMORY = false;

  struct RuntimeParams {
    SCALE_TIE(4);
    kagome::runtime::RuntimeContext::ContextParams context_params;
    uint64_t pvf_exec_timeout_approval_ms = DEFAULT_APPROVAL_EXECUTION_TIMEOUT;
    uint64_t pvf_exec_timeout_backing_ms = DEFAULT_BACKING_EXECUTION_TIMEOUT;
    bool wasm_ext_bulk_memory = DEFAULT_WASM_EXT_BULK_MEMORY;
  };

}  // namespace kagome::parachain

SCALE_TIE_HASH_STD(kagome::parachain::RuntimeParams);
