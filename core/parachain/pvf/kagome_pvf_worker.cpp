/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <filesystem>
#include <iostream>
#include <memory>
#include <ranges>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#ifdef __linux__
#include <linux/landlock.h>
#include <sched.h>
#include <seccomp.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#endif

#include <fmt/format.h>
#include <boost/asio.hpp>
#include <boost/process.hpp>
#include <libp2p/basic/scheduler/asio_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>
#include <libp2p/common/asio_buffer.hpp>
#include <libp2p/common/final_action.hpp>
#include <libp2p/log/configurator.hpp>
#include <soralog/macro.hpp>

#include "common/bytestr.hpp"
#include "log/configurator.hpp"
#include "log/logger.hpp"
#include "log/profiling_logger.hpp"
#include "parachain/pvf/kagome_pvf_worker_injector.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "scale/scale.hpp"

#include "parachain/pvf/kagome_pvf_worker.hpp"
#include "parachain/pvf/secure_mode.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/wabt/instrument.hpp"
#include "utils/mkdirs.hpp"

// rust reference: polkadot-sdk/polkadot/node/core/pvf/execute-worker/src/lib.rs

#define EXPECT_NON_NEG(func, ...)                  \
  if (auto res = ::func(__VA_ARGS__); res == -1) { \
    return ::kagome::parachain::getLastErr(#func); \
  }

#define EXPECT_NON_NULL(func, args)                \
  if (auto res = func args; res == nullptr) {      \
    return ::kagome::parachain::getLastErr(#func); \
  }

namespace kagome::parachain {
  static kagome::log::Logger logger;

  bool checkEnvVarsEmpty(const char **env) {
    return env != nullptr;
  }

#ifdef __linux__

  SecureModeError getLastErr(std::string_view call_name) {
    return SecureModeError{
        fmt::format("{} failed: {}", call_name, strerror(errno))};
  }

  // This should not be called in a multi-threaded context. `unshare(2)`:
  // "CLONE_NEWUSER requires that the calling process is not threaded."
  SecureModeOutcome<void> changeRoot(const std::filesystem::path &worker_dir) {
    OUTCOME_TRY(mkdirs(worker_dir));

    EXPECT_NON_NEG(unshare, CLONE_NEWUSER | CLONE_NEWNS);
    EXPECT_NON_NEG(mount, nullptr, "/", nullptr, MS_REC | MS_PRIVATE, nullptr);

    EXPECT_NON_NEG(
        mount,
        worker_dir.c_str(),
        worker_dir.c_str(),
        nullptr,
        MS_BIND | MS_REC | MS_NOEXEC | MS_NODEV | MS_NOSUID | MS_NOATIME,
        nullptr);
    EXPECT_NON_NEG(chdir, (worker_dir.c_str()));

    EXPECT_NON_NEG(syscall, SYS_pivot_root, ".", ".");
    EXPECT_NON_NEG(umount2, ".", MNT_DETACH);
    if (std::filesystem::current_path() != "/") {
      return SecureModeError{"Chroot failed: . is not /"};
    }
    std::error_code err{};
    std::filesystem::current_path("..", err);
    if (err) {
      return SecureModeError{fmt::format("Failed to chdir to ..: {}", err)};
    }
    if (std::filesystem::current_path() != "/") {
      return SecureModeError{
          fmt::format("Successfully escaped from chroot with 'chdir ..'")};
    }
    return outcome::success();
  }

  SecureModeOutcome<void> enableSeccomp() {
    std::array forbidden_calls{
        SCMP_SYS(socketpair),
        SCMP_SYS(socket),
        SCMP_SYS(connect),
        SCMP_SYS(io_uring_setup),
        SCMP_SYS(io_uring_enter),
        SCMP_SYS(io_uring_register),
    };
    auto scmp_ctx = seccomp_init(SCMP_ACT_ALLOW);
    if (!scmp_ctx) {
      return getLastErr("seccomp_init");
    }
    libp2p::common::FinalAction cleanup{
        [scmp_ctx]() { seccomp_release(scmp_ctx); }};

    for (auto &call : forbidden_calls) {
      EXPECT_NON_NEG(
          seccomp_rule_add, scmp_ctx, SCMP_ACT_KILL_PROCESS, call, 0);
    }

    EXPECT_NON_NEG(seccomp_load, scmp_ctx);

    return outcome::success();
  }

  SecureModeOutcome<void> enableLandlock(
      const std::filesystem::path &worker_dir) {
    std::array<std::pair<std::filesystem::path, uint64_t>, 1>
        allowed_exceptions;
    // TODO(Harrm): #2103 Separate PVF workers on prepare and execute workers,
    // and separate FS permissions accordingly
    allowed_exceptions[0] =
        std::pair{worker_dir,
                  LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_WRITE_FILE
                      | LANDLOCK_ACCESS_FS_MAKE_REG};

    int abi{};

    abi = ::syscall(
        SYS_landlock_create_ruleset, NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);
    if (abi < 0) {
      return getLastErr("landlock_create_ruleset");
    }

    struct landlock_ruleset_attr ruleset_attr = {
        .handled_access_fs =
            LANDLOCK_ACCESS_FS_EXECUTE | LANDLOCK_ACCESS_FS_WRITE_FILE
            | LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR
            | LANDLOCK_ACCESS_FS_REMOVE_DIR | LANDLOCK_ACCESS_FS_REMOVE_FILE
            | LANDLOCK_ACCESS_FS_MAKE_CHAR | LANDLOCK_ACCESS_FS_MAKE_DIR
            | LANDLOCK_ACCESS_FS_MAKE_REG | LANDLOCK_ACCESS_FS_MAKE_SOCK
            | LANDLOCK_ACCESS_FS_MAKE_FIFO | LANDLOCK_ACCESS_FS_MAKE_BLOCK
            | LANDLOCK_ACCESS_FS_MAKE_SYM
#ifdef LANDLOCK_ACCESS_FS_REFER
            | LANDLOCK_ACCESS_FS_REFER
#endif
#ifdef LANDLOCK_ACCESS_FS_TRUNCATE
            | LANDLOCK_ACCESS_FS_TRUNCATE
#endif
        ,
#ifdef LANDLOCK_ACCESS_NET_CONNECT_TCP
        .handled_access_net =
            LANDLOCK_ACCESS_NET_BIND_TCP | LANDLOCK_ACCESS_NET_CONNECT_TCP,
#endif
    };

    int ruleset_fd{};

    ruleset_fd = ::syscall(
        SYS_landlock_create_ruleset, &ruleset_attr, sizeof(ruleset_attr), 0);
    if (ruleset_fd < 0) {
      return getLastErr("landlock_create_ruleset");
    }
    libp2p::common::FinalAction cleanup = [ruleset_fd]() { close(ruleset_fd); };

    for (auto &[path, access_flags] : allowed_exceptions) {
      struct landlock_path_beneath_attr path_beneath = {
          .allowed_access = access_flags,
          .parent_fd = ::open(path.c_str(), O_PATH | O_CLOEXEC),
      };

      if (path_beneath.parent_fd < 0) {
        return getLastErr("open");
      }
      auto err = ::syscall(SYS_landlock_add_rule,
                           ruleset_fd,
                           LANDLOCK_RULE_PATH_BENEATH,
                           &path_beneath,
                           0);
      ::close(path_beneath.parent_fd);
      if (err == -1) {
        return getLastErr("landlock_add_rule");
      }
    }

    if (::prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)
        == -1) {  // NOLINT(cppcoreguidelines-pro-type-vararg)
      ::close(ruleset_fd);
      return getLastErr("prctl PR_SET_NO_NEW_PRIVS");
    }

    if (::syscall(SYS_landlock_restrict_self, ruleset_fd, 0)) {
      return getLastErr("landlock_restrict_self");
    }

    return outcome::success();
  }
#endif

  outcome::result<void> readStdin(std::span<uint8_t> out) {
    std::cin.read(reinterpret_cast<char *>(out.data()), out.size());
    if (not std::cin.good()) {
      return std::errc::io_error;
    }
    return outcome::success();
  }

  template <typename T>
  outcome::result<T> decodeInput() {
    std::array<uint8_t, sizeof(uint32_t)> length_bytes;
    OUTCOME_TRY(readStdin(length_bytes));
    OUTCOME_TRY(message_length, scale::decode<uint32_t>(length_bytes));
    std::vector<uint8_t> packed_message(message_length, 0);
    OUTCOME_TRY(readStdin(packed_message));
    return scale::decode<T>(packed_message);
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
    OUTCOME_TRY(input_config, decodeInput<PvfWorkerInputConfig>());
    kagome::log::tuneLoggingSystem(input_config.log_params);

    SL_VERBOSE(logger, "Cache directory: {}", input_config.cache_dir);
    if (not std::filesystem::path{input_config.cache_dir}.is_absolute()) {
      SL_ERROR(
          logger, "cache dir must be absolute: {}", input_config.cache_dir);
      return std::errc::invalid_argument;
    }

    std::function<outcome::result<std::filesystem::path>(
        const std::filesystem::path &)>
        chroot_path = [](const std::filesystem::path &path) { return path; };
#ifdef __linux__
    if (!input_config.force_disable_secure_mode) {
      auto root = input_config.cache_dir;
      if (root.ends_with("/")) {
        root.pop_back();
      }
      chroot_path = [=](const std::filesystem::path &path)
          -> outcome::result<std::filesystem::path> {
        std::string_view s{path.native()};
        if (s == root) {
          return std::filesystem::path{"/"};
        }
        if (not s.starts_with(root)
            or not s.substr(root.size()).starts_with("/")) {
          SL_ERROR(logger, "path outside chroot: {}", s);
          return std::errc::permission_denied;
        }
        return std::filesystem::path{s.substr(root.size())};
      };
      SL_VERBOSE(logger, "Attempting to enable secure validator mode...");

      if (auto res = changeRoot(root); !res) {
        SL_ERROR(logger,
                 "Failed to enable secure validator mode (change root): {}",
                 res.error());
        return std::errc::not_supported;
      }

      OUTCOME_TRY(path, chroot_path(input_config.cache_dir));
      if (auto res = enableLandlock(path); !res) {
        SL_ERROR(logger,
                 "Failed to enable secure validator mode (landlock): {}",
                 res.error());
        return std::errc::not_supported;
      }
      if (auto res = enableSeccomp(); !res) {
        SL_ERROR(logger,
                 "Failed to enable secure validator mode (seccomp): {}",
                 res.error());
        return std::errc::not_supported;
      }
      SL_VERBOSE(logger, "Successfully enabled secure validator mode");
    } else {
      SL_VERBOSE(logger,
                 "Secure validator mode disabled in node configuration");
    }
#endif
    auto injector = pvf_worker_injector(input_config);
    OUTCOME_TRY(factory, createModuleFactory(injector, input_config.engine));
    std::shared_ptr<runtime::Module> module;
    while (true) {
      KAGOME_PROFILE_START(pvf_worker_loop);

      KAGOME_PROFILE_START(pvf_read_input);
      OUTCOME_TRY(input, decodeInput<PvfWorkerInput>());
      KAGOME_PROFILE_END(pvf_read_input);

      if (auto *code_path = std::get_if<RuntimeCodePath>(&input)) {
        KAGOME_PROFILE_START(pvf_load_code);
        OUTCOME_TRY(path, chroot_path(*code_path));
        BOOST_OUTCOME_TRY(module, factory->loadCompiled(path));
        continue;
      }
      auto &input_args = std::get<PvfWorkerInputArgs>(input);
      if (not module) {
        SL_ERROR(logger, "PvfWorkerInputCode expected");
        return std::errc::invalid_argument;
      }
      OUTCOME_TRY(instance, module->instantiate());

      OUTCOME_TRY(ctx, runtime::RuntimeContextFactory::stateless(instance));
      OUTCOME_TRY(
          result,
          instance->callExportFunction(ctx, "validate_block", input_args));
      OUTCOME_TRY(instance->resetEnvironment());
      OUTCOME_TRY(len, scale::encode<uint32_t>(result.size()));
      KAGOME_PROFILE_START(pvf_write_output);
      std::cout.write((const char *)len.data(), len.size());
      std::cout.write((const char *)result.data(), result.size());
      std::cout.flush();
    }
  }

  int pvf_worker_main(int argc, const char **argv, const char **env) {
    auto logging_system = std::make_shared<soralog::LoggingSystem>(
        std::make_shared<kagome::log::Configurator>(
            std::make_shared<libp2p::log::Configurator>()));
    auto r = logging_system->configure();
    if (not r.message.empty()) {
      std::cerr << r.message << '\n';
    }
    if (r.has_error) {
      return EXIT_FAILURE;
    }
    kagome::log::setLoggingSystem(logging_system);
    logger = kagome::log::createLogger("PVF Worker", "parachain");

    if (!checkEnvVarsEmpty(env)) {
      logger->error(
          "PVF worker processes must not have any environment variables.");
      return EXIT_FAILURE;
    }

    if (auto r = pvf_worker_main_outcome(); not r) {
      SL_ERROR(logger, "PVF worker process failed: {}", r.error());
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }
}  // namespace kagome::parachain
