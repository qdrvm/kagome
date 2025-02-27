/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "pvm/config.hpp"
#include "pvm/sandbox.hpp"
#include "pvm/types.hpp"

namespace kagome::pvm {

  struct EngineState {
    Opt<sandbox::GlobalStateKind> sandbox_global;

    // TODO(iceseer)
    // Opt<sandbox::WorkerCacheKind> sandbox_cache;
    // CompilerCache compiler_cache;
    // ModuleCache _module_cache;
  };

  struct Engine {
    BackendKind selected_backend;
    Opt<SandboxKind> selected_sandbox;
    std::shared_ptr<EngineState> state;

    bool allow_dynamic_paging;
    bool interpreter_enabled;
    bool crosscheck;

    static outcome::result<Engine> create(const Config &config) {
      sandbox::init_native_page_size();
      if (config.backend) {
        const auto &backend = *config.backend;
        if (!backend.is_supported()) {
          return Error::UNSUPPORTED_BACKEND_KIND;
        }
      }

      if (!config.allow_experimental && config.crosscheck) {
        return Error::ALLOW_EXPERIMENTAL_DISABLED;
      }

      const auto crosscheck = config.crosscheck;
      const auto selected_backend = [&]() {
        if (config.backend) {
          return *config.backend;
        }
        return BackendKind{.value = BackendKind::Compiler};
      }();

      if (config.cache_enabled) {
        return Error::MODULE_CACHE_IS_NOT_SUPPORTED;
      }

      if (selected_backend.value == BackendKind::Compiler) {
        const auto selected_sandbox = [&]() {
          if (config.sandbox) {
            return *config.sandbox;
          }
          if (SandboxKind::is_supported(SandboxKind::Linux)) {
            return SandboxKind{.value = SandboxKind::Linux};
          }
          return SandboxKind{.value = SandboxKind::Generic};
        }();

        if (!selected_sandbox.is_supported()) {
          return Error::UNSUPPORTED_SANDBOX;
        }

        if (selected_sandbox.value == SandboxKind::Generic
            && !config.allow_experimental) {
          return Error::ALLOW_EXPERIMENTAL_DISABLED;
        }

        // TODO(iceseer)
        // auto sandbox_global = sandbox::GlobalStateKind::new(selected_sandbox,
        // config)?; auto sandbox_cache =
        // sandbox::WorkerCacheKind::new(selected_sandbox, config); for _ in
        // 0..config.worker_count {
        //     sandbox_cache.spawn(&sandbox_global)?;
        // }

        // let state = Arc::new(EngineState {
        //     sandbox_global: Some(sandbox_global),
        //     sandbox_cache: Some(sandbox_cache),
        //     compiler_cache: Default::default(),

        //     #[cfg(feature = "module-cache")]
        //     module_cache,
        // });

        // (Some(selected_sandbox), state)
      } else {
        // (None, Arc::new(EngineState {
        //     sandbox_global: None,
        //     sandbox_cache: None,
        //     compiler_cache: Default::default(),

        //     #[cfg(feature = "module-cache")]
        //     module_cache
        // }))
      }

      return Error::NOT_IMPLEMENTED;
    }

   private:
    Engine() = default;
  };

}  // namespace kagome::pvm
