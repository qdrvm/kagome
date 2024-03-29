/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <filesystem>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

#ifdef __linux__
#include <sched.h>
#include <sys/mount.h>
#include <linux/seccomp.h>
#endif

#include <fmt/format.h>
#include <libp2p/log/configurator.hpp>

#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/common/asio_buffer.hpp>

#include "common/bytestr.hpp"
#include "log/configurator.hpp"
#include "log/logger.hpp"
#include "parachain/pvf/kagome_pvf_worker_injector.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "scale/scale.hpp"

#include "parachain/pvf/kagome_pvf_worker.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"

// rust reference: polkadot-sdk/polkadot/node/core/pvf/execute-worker/src/lib.rs

bool check_env_vars_are_empty(const char **env) {
  if (env != nullptr) {
    std::cout << "Env variables are set!:\n";
    while (env) {
      std::cout << *env << "\n";
      env++;
    }
    std::cout
        << "All env variables should be unset for PVF workers for security reasons\n";
    return false;
  }
  return true;
}

std::error_code get_last_err() {
  return std::error_code{static_cast<std::errc>(errno)};
}

enum class WorkerKind { Prepare, Execute, CheckPivotRoot };

// This should not be called in a multi-threaded context. `unshare(2)`:
// "CLONE_NEWUSER requires that the calling process is not threaded."
std::error_code change_root(const std::filesystem::path &worker_dir,
                            WorkerKind kind) {
  if (::unshare(CLONE_NEWUSER | CLONE_NEWNS) == -1) {
    return get_last_err();
  }
  if (::mount(nullptr, "/", nullptr, MS_REC | MS_PRIVATE, nullptr) == -1) {
    return get_last_err();
  }
  int additional_flags = 0;
  if (kind == WorkerKind::Prepare || kind == WorkerKind::CheckPivotRoot) {
    additional_flags = MS_RDONLY;
  }
  if (::mount(worker_dir.c_str(),
              worker_dir.c_str(),
              nullptr,
              MS_BIND | MS_REC | MS_NOEXEC | MS_NODEV | MS_NOSUID | MS_NOATIME
                  | additional_flags,
              nullptr)) {
    return get_last_err();
  }
  if (::chdir(worker_dir.c_str()) == -1) {
    return get_last_err();
  }
  if (::syscall(SYS_pivot_root, ".", ".") == -1) {
    return get_last_err();
  }
  if (::umount2(".", MNT_DETACH) == -1) {
    return get_last_err();
  }
  if (std::filesystem::current_path() != "/") {
    return Error("Failed to change process mount root");
  }
  std::error_code err{};
  std::filesystem::current_path("..", err);
  if (err) {
    return err;
  }
  if (std::filesystem::current_path() != "/") {
    return Error(
        "Successfully broke out of new root using .., which must not happen");
  }
  return {};
}

void enable_seccomp() {
  std::array forbidden_calls {
    SYS_socketpair,
    SYS_socket,
    SYS_connect,
    SYS_io_uring_setup,
    SYS_io_uring_enter,
    SYS_io_uring_register,
  };

  // maybe 
  if(prctl(PR_SET_NO_NEW_PRIVS, 1) == -1) {
    return get_last_err();
  }
  struct sock_filter filter[] {
    {},
  };
  struct sock_fprog filter_rules {

  };
  if(syscall(SYS_seccomp, SECCOMP_SET_MODE_FILTER, 0, ) == -1) {
    return get_last_err();
  }
}

void enable_landlock() {}

namespace kagome::parachain {
  static kagome::log::Logger logger;

  outcome::result<void> readStdin(std::span<uint8_t> out) {
    std::cin.read(reinterpret_cast<char *>(out.data()), out.size());
    if (not std::cin.good()) {
      return std::errc::io_error;
    }
    return outcome::success();
  }

  outcome::result<PvfWorkerInput> decodeInput() {
    std::array<uint8_t, sizeof(uint32_t)> length_bytes;
    OUTCOME_TRY(readStdin(length_bytes));
    OUTCOME_TRY(message_length, scale::decode<uint32_t>(length_bytes));
    std::vector<uint8_t> packed_message(message_length, 0);
    OUTCOME_TRY(readStdin(packed_message));
    OUTCOME_TRY(pvf_worker_input,
                scale::decode<PvfWorkerInput>(packed_message));
    return pvf_worker_input;
  }

  outcome::result<std::shared_ptr<runtime::ModuleFactory>> createModuleFactory(
      const auto &injector, RuntimeEngine engine) {
    switch (engine) {
      case RuntimeEngine::kBinaryen:
        return injector.template create<
            std::shared_ptr<runtime::binaryen::ModuleFactoryImpl>>();
      case RuntimeEngine::kWAVM:
        // About ifdefs - looks bad, but works as it ought to
#if KAGOME_WASM_COMPILER_WAVM == 1
        return injector.template create<
            std::shared_ptr<runtime::wavm::ModuleFactoryImpl>>();
#else
        SL_ERROR(logger, "WAVM runtime engine is not supported");
        return std::errc::not_supported;
#endif
      case RuntimeEngine::kWasmEdgeInterpreted:
      case RuntimeEngine::kWasmEdgeCompiled:
#if KAGOME_WASM_COMPILER_WASM_EDGE == 1
        return injector.template create<
            std::shared_ptr<runtime::wasm_edge::ModuleFactoryImpl>>();
#else
        SL_ERROR(logger, "WasmEdge runtime engine is not supported");
        return std::errc::not_supported;
#endif
      default:
        SL_ERROR(logger, "Unknown runtime engine is requested");
        return std::errc::not_supported;
    }
  }

  outcome::result<void> pvf_worker_main_outcome() {
    OUTCOME_TRY(input, decodeInput());
    kagome::log::tuneLoggingSystem(input.log_params);
    auto injector = pvf_worker_injector(input);
    OUTCOME_TRY(factory, createModuleFactory(injector, input.engine));
    OUTCOME_TRY(ctx,
                runtime::RuntimeContextFactory::fromCode(
                    *factory, input.runtime_code, input.runtime_params));
    OUTCOME_TRY(result,
                ctx.module_instance->callExportFunction(
                    ctx, input.function, input.params));
    OUTCOME_TRY(ctx.module_instance->resetEnvironment());
    OUTCOME_TRY(len, scale::encode<uint32_t>(result.size()));
    std::cout.write((const char *)len.data(), len.size());
    std::cout.write((const char *)result.data(), result.size());
    return outcome::success();
  }

  int pvf_worker_main(int argc, const char **argv, const char **env) {
    if (!check_env_vars_are_empty(env)) {
      return -1;
    }
    auto logging_system = std::make_shared<soralog::LoggingSystem>(
        std::make_shared<kagome::log::Configurator>(
            std::make_shared<libp2p::log::Configurator>()));
    auto r = logging_system->configure();
    if (not r.message.empty()) {
      std::cerr << r.message << std::endl;
    }
    if (r.has_error) {
      return EXIT_FAILURE;
    }
    kagome::log::setLoggingSystem(logging_system);
    logger = kagome::log::createLogger("Pvf Worker", "parachain");

    if (auto r = pvf_worker_main_outcome(); not r) {
      SL_ERROR(logger, "{}", r.error());
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }
}  // namespace kagome::parachain
