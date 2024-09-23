#pragma once

#include "runtime/runtime_context.hpp"

namespace kagome::parachain {
  static constexpr uint64_t DEFAULT_PVF_EXECUTOR_TIMEOUT_MS = 2000;

  struct RuntimeParams {
    SCALE_TIE(2);
    kagome::runtime::RuntimeContext::ContextParams context_params;
    uint64_t pvf_exec_timeout_backing_ms = DEFAULT_PVF_EXECUTOR_TIMEOUT_MS;
  };

}  // namespace kagome::parachain

SCALE_TIE_HASH_STD(kagome::parachain::RuntimeParams);
