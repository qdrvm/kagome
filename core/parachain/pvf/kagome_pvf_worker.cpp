/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <filesystem>
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
#include <boost/asio/local/stream_protocol.hpp>
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
#include "parachain/pvf/clone.hpp"
#include "parachain/pvf/kagome_pvf_worker.hpp"
#include "parachain/pvf/kagome_pvf_worker_injector.hpp"
#include "parachain/pvf/pvf_worker_types.hpp"
#include "parachain/pvf/secure_mode.hpp"
#include "runtime/binaryen/module/module_factory_impl.hpp"
#include "runtime/module_instance.hpp"
#include "runtime/runtime_context.hpp"
#include "runtime/wabt/instrument.hpp"
#include "runtime/wasm_compiler_definitions.hpp"  // this header-file is generated
#include "scale/kagome_scale.hpp"
#include "utils/mkdirs.hpp"
#include "utils/spdlog_stderr.hpp"

// NOLINTBEGIN(cppcoreguidelines-pro-type-vararg,hicpp-vararg)

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
  using unix = boost::asio::local::stream_protocol;

  namespace {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    static kagome::log::Logger logger = [] {
      return kagome::log::createLogger("PVF Worker", "parachain");
    }();
  }  // namespace

  bool checkEnvVarsEmpty(const char **env) {
#ifdef KAGOME_WITH_ASAN
    //  explicitly allow to disable LSAN, because LSAN doesn't work in secure
    //  mode, since it wants to access /proc
    if (*env != nullptr
        && "ASAN_OPTIONS=detect_leaks=0" == std::string_view{*env}) {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      ++env;
    }
#endif
    return *env == nullptr;
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

    auto abi = ::syscall(
        SYS_landlock_create_ruleset, NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);
    auto logger = log::createLogger("Landlock", "parachain");
    SL_INFO(logger, "Landlock ABI version: {} ", abi);
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
            | LANDLOCK_ACCESS_FS_MAKE_SYM};

    // only add Landlock V2+ features if defined and supported by the (runtime)
    // kernel
#ifdef LANDLOCK_ACCESS_FS_REFER
    if (abi >= 2) {
      ruleset_attr.handled_access_fs |= LANDLOCK_ACCESS_FS_REFER;
    }
#endif
#ifdef LANDLOCK_ACCESS_FS_TRUNCATE
    if (abi >= 3) {
      ruleset_attr.handled_access_fs |= LANDLOCK_ACCESS_FS_TRUNCATE;
    }
#endif
#ifdef LANDLOCK_ACCESS_NET_CONNECT_TCP
    if (abi >= 4) {
      ruleset_attr.handled_access_net =
          LANDLOCK_ACCESS_NET_BIND_TCP | LANDLOCK_ACCESS_NET_CONNECT_TCP;
    }
#endif

    auto ruleset_fd = ::syscall(
        SYS_landlock_create_ruleset, &ruleset_attr, sizeof(ruleset_attr), 0);
    if (ruleset_fd < 0) {
      return getLastErr("landlock_create_ruleset");
    }
    libp2p::common::FinalAction cleanup = [ruleset_fd]() {
      close(ruleset_fd);  // NOLINT(cppcoreguidelines-narrowing-conversions)
    };

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

    if (::prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == -1) {
      // NOLINTNEXTLINE(cppcoreguidelines-narrowing-conversions)
      ::close(ruleset_fd);
      return getLastErr("prctl PR_SET_NO_NEW_PRIVS");
    }

    if (::syscall(SYS_landlock_restrict_self, ruleset_fd, 0) != 0) {
      return getLastErr("landlock_restrict_self");
    }

    return outcome::success();
  }
#endif

  template <typename T>
  outcome::result<T> decodeInput(unix::socket &socket) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,hicpp-member-init)
    std::array<uint8_t, sizeof(uint32_t)> length_bytes;
    boost::system::error_code ec;
    boost::asio::read(socket, boost::asio::buffer(length_bytes), ec);
    if (ec) {
      return ec;
    }
    OUTCOME_TRY(message_length, scale::decode<uint32_t>(length_bytes));
    std::vector<uint8_t> packed_message(message_length, 0);
    boost::asio::read(socket, boost::asio::buffer(packed_message), ec);
    if (ec) {
      return ec;
    }
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

  outcome::result<void> pvf_worker_main_outcome(
      const std::string &unix_socket_path) {
    boost::asio::io_context io_context;
    unix::socket socket{io_context};
    boost::system::error_code ec;
    socket.connect(unix_socket_path, ec);
    if (ec) {
      return ec;
    }
    OUTCOME_TRY(input_config, decodeInput<PvfWorkerInputConfig>(socket));
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
      OUTCOME_TRY(input, decodeInput<PvfWorkerInput>(socket));

      if (auto *code_params = std::get_if<PvfWorkerInputCodeParams>(&input)) {
        auto &path = code_params->path;
        BOOST_OUTCOME_TRY(path, chroot_path(path));
        BOOST_OUTCOME_TRY(
            module, factory->loadCompiled(path, code_params->context_params));
        continue;
      }
      auto &input_args = std::get<PvfWorkerInputArgs>(input);
      if (not module) {
        SL_ERROR(logger, "PvfWorkerInputCodeParams expected");
        return std::errc::invalid_argument;
      }
      auto forked = [&]() -> outcome::result<void> {
        OUTCOME_TRY(instance, module->instantiate());

        OUTCOME_TRY(ctx, runtime::RuntimeContextFactory::stateless(instance));
        OUTCOME_TRY(
            result,
            instance->callExportFunction(ctx, "validate_block", input_args));
        OUTCOME_TRY(instance->resetEnvironment());
        OUTCOME_TRY(len, scale::encode<uint32_t>(result.size()));

        boost::asio::write(socket, boost::asio::buffer(len), ec);
        if (ec) {
          return ec;
        }
        boost::asio::write(socket, boost::asio::buffer(result), ec);
        if (ec) {
          return ec;
        }
        return outcome::success();
      };
      OUTCOME_TRY(clone::cloneOrFork(logger, input_config, forked));
    }
  }

  int pvf_worker_main(int argc, const char **argv, const char **env) {
    spdlogStderr();

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
    if (argc < 2) {
      SL_ERROR(logger, "missing unix socket path arg");
      return EXIT_FAILURE;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (auto r = pvf_worker_main_outcome(argv[1]); not r) {
      SL_ERROR(logger, "PVF worker process failed: {}", r.error());
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }
}  // namespace kagome::parachain

// NOLINTEND(cppcoreguidelines-pro-type-vararg,hicpp-vararg)
