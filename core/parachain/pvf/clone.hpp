/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef __linux__
#include <sched.h>
#include <signal.h>
#endif

#include "parachain/pvf/pvf_worker_types.hpp"

namespace kagome::parachain::clone {
  constexpr size_t kCloneStackSize = 2 << 20;

  enum class CloneError : uint8_t {
    kCallbackFailed,
  };
  Q_ENUM_ERROR_CODE(CloneError) {
    using E = decltype(e);
    switch (e) {
      case E::kCallbackFailed:
        return "Callback failed";
    }
    abort();
  }

#ifdef __linux__
  // https://github.com/paritytech/polkadot-sdk/blob/f4a196ab1473856c9c5992239fcc2f14c2c42914/polkadot/node/core/pvf/common/src/worker/security/clone.rs#L35-L54
  /// Try to run clone(2) on the current worker.
  ///
  /// SAFETY: new process should be either spawned within a single threaded
  /// process, or use only async-signal-safe functions.
  template <typename Cb>
  inline outcome::result<pid_t> clone(bool have_unshare_newuser, const Cb &cb) {
    Buffer stack(kCloneStackSize);
    // https://github.com/paritytech/polkadot-sdk/blob/f4a196ab1473856c9c5992239fcc2f14c2c42914/polkadot/node/core/pvf/common/src/worker/security/clone.rs#L75-L93
    int flags = CLONE_NEWCGROUP | CLONE_NEWIPC | CLONE_NEWNET | CLONE_NEWNS
              | CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD;
    if (not have_unshare_newuser) {
      flags |= CLONE_NEWUSER;
    }
    pid_t pid = ::clone(
        [](void *arg) {
          auto &cb = *reinterpret_cast<const Cb *>(arg);
          return cb() ? EXIT_SUCCESS : EXIT_FAILURE;
        },
        stack.data() + stack.size(),
        flags,
        const_cast<void *>(static_cast<const void *>(&cb)));
    if (pid == -1) {
      return std::errc{errno};
    }
    return pid;
  }
#endif

  inline outcome::result<void> wait(pid_t pid) {
    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
      return std::errc{errno};
    }
    if (not WIFEXITED(status) or WEXITSTATUS(status) != EXIT_SUCCESS) {
      return CloneError::kCallbackFailed;
    }
    return outcome::success();
  }

  // https://github.com/paritytech/polkadot-sdk/blob/f4a196ab1473856c9c5992239fcc2f14c2c42914/polkadot/node/core/pvf/execute-worker/src/lib.rs#L245-L293
  /// Call callback either directly, or inside `clone`, or inside `fork`.
  inline outcome::result<void> cloneOrFork(const log::Logger &log,
                                           const PvfWorkerInputConfig &config,
                                           const auto &cb) {
    auto cb_log = [&] {
      auto r = cb();
      if (not r) {
        SL_WARN(log, "cloneOrFork cb returned error: {}", r.error());
        return false;
      }
      return true;
    };
    if (config.force_disable_secure_mode) {
      if (not cb_log()) {
        return CloneError::kCallbackFailed;
      }
      return outcome::success();
    }
    std::optional<pid_t> pid;
#ifdef __linux__
    if (config.secure_mode_support.can_do_secure_clone) {
      BOOST_OUTCOME_TRY(pid, clone(config.secure_mode_support.chroot, cb_log));
    }
#endif
    if (not pid) {
      pid = fork();
      if (pid == -1) {
        return std::errc{errno};
      }
      if (pid == 0) {
        _Exit(cb_log() ? EXIT_SUCCESS : EXIT_FAILURE);
      }
    }
    return wait(*pid);
  }

  // https://github.com/paritytech/polkadot-sdk/blob/f4a196ab1473856c9c5992239fcc2f14c2c42914/polkadot/node/core/pvf/common/src/worker/security/clone.rs#L56-L63
  /// Runs a check for clone(2) with all sandboxing flags and returns an error
  /// indicating whether it can be fully enabled on the current Linux
  /// environment.
  ///
  /// SAFETY: new process should be either spawned within a single threaded
  /// process, or use only async-signal-safe functions.
  inline outcome::result<void> check() {
#ifdef __linux__
    OUTCOME_TRY(pid, clone(false, [] { return true; }));
    return wait(pid);
#else
    return std::errc::not_supported;
#endif
  }
}  // namespace kagome::parachain::clone
