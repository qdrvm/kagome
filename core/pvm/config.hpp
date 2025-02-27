/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <type_traits>
#include "primitives/math.hpp"
#include "pvm/types.hpp"

template <size_t N>
inline constexpr std::string_view into_string_view(const char (&value)[N]) {
  return std::string_view(value, N - 1);
}

inline constexpr std::optional<std::string_view> str_from_env(
    const std::string_view &key) {
  if (auto value = std::getenv(key.data())) {
    return std::string_view(value);
  }
  return std::nullopt;
}

inline constexpr std::optional<int64_t> ll_from_env(
    const std::string_view &key) {
  if (auto value = std::getenv(key.data())) {
    if (value[0] >= '0' && value[0] <= '9') {
      return std::atoll(value);
    }
  }
  return std::nullopt;
}

inline constexpr std::optional<bool> bool_from_env(
    const std::string_view &key) {
  if (auto value = str_from_env(key)) {
    if (value == "1" || value == "true") {
      return true;
    }
    if (value == "0" || value == "false") {
      return false;
    }
  }
  return std::nullopt;
}

namespace kagome::pvm {
  struct BackendKind {
    enum T : uint8_t {
      Compiler,
      Interpreter,
    } value;

    static outcome::result<Opt<BackendKind>> from_str(
        const std::string_view &s) {
      if (s == into_string_view("auto")) {
        return std::nullopt;
      } else if (s == into_string_view("interpreter")) {
        return BackendKind{.value = Interpreter};
      } else if (s == into_string_view("compiler")) {
        return BackendKind{.value = Compiler};
      } else {
        return Error::UNSUPPORTED_BACKEND_KIND;
      }
    }

    bool is_supported() const {
      // Depends on compiler.
      return true;
    }
  };

  struct SandboxKind {
    enum T : uint8_t {
      Linux,
      Generic,
    } value;

    static outcome::result<Opt<SandboxKind>> from_str(
        const std::string_view &s) {
      if (s == into_string_view("auto")) {
        return std::nullopt;
      } else if (s == into_string_view("linux")) {
        return SandboxKind{.value = Linux};
      } else if (s == into_string_view("generic")) {
        return SandboxKind{.value = Generic};
      } else {
        return Error::UNSUPPORTED_SANDBOX;
      }
    }

    static bool is_supported(T value) {
#if defined(__linux__)
      const auto builded = Linux;
#else
      const auto builded = Generic;
#endif
      return builded == value;
    }

    bool is_supported() const {
      return SandboxKind::is_supported(value);
    }
  };

  struct Config {
    Opt<BackendKind> backend = std::nullopt;
    Opt<SandboxKind> sandbox = std::nullopt;
    bool crosscheck = false;
    bool allow_experimental = false;
    bool allow_dynamic_paging = false;
    size_t worker_count = 2;
    bool cache_enabled = false;
    uint32_t lru_cache_size = 0;

    static outcome::result<Config> from_env() {
      Config config;

      if (const auto value =
              str_from_env(into_string_view("POLKAVM_BACKEND"))) {
        OUTCOME_TRY(backend, BackendKind::from_str(*value));
        config.backend = backend;
      }

      if (const auto value =
              str_from_env(into_string_view("POLKAVM_SANDBOX"))) {
        OUTCOME_TRY(sandbox, SandboxKind::from_str(*value));
        config.sandbox = sandbox;
      }

      if (const auto value =
              bool_from_env(into_string_view("POLKAVM_CROSSCHECK"))) {
        config.crosscheck = *value;
      }

      if (const auto value =
              bool_from_env(into_string_view("POLKAVM_ALLOW_EXPERIMENTAL"))) {
        config.allow_experimental = *value;
      }

      if (const auto value =
              ll_from_env(into_string_view("POLKAVM_WORKER_COUNT"))) {
        config.worker_count = size_t(*value);
      }

      if (const auto value =
              bool_from_env(into_string_view("POLKAVM_CACHE_ENABLED"))) {
        config.cache_enabled = size_t(*value);
      }

      if (const auto value =
              ll_from_env(into_string_view("POLKAVM_LRU_CACHE_SIZE"))) {
        config.lru_cache_size = ((*value > std::numeric_limits<uint32_t>::max())
                                     ? std::numeric_limits<uint32_t>::max()
                                     : uint32_t(*value));
      }

      return config;
    }
  };
}  // namespace kagome::pvm
