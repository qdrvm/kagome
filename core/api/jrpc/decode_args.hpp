/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <jsonrpc-lean/request.h>

#include "primitives/mmr.hpp"

namespace kagome::api::details {
  struct LoadValue {
    static void throwInvalidType() {
      throw jsonrpc::InvalidParametersFault{"invalid argument type"};
    }

    static void throwInvalidValue() {
      throw jsonrpc::InvalidParametersFault{"invalid argument value"};
    }

    template <typename T>
    static T unwrap(outcome::result<T> &&r) {
      if (r) {
        return std::move(r.value());
      }
      throw jsonrpc::InvalidParametersFault{r.error().message()};
    }

    static auto &mapAt(const jsonrpc::Value &j, const std::string &k) {
      auto &m = j.AsStruct();
      if (auto it = m.find(k); it != m.end()) {
        return it->second;
      }
      throw jsonrpc::InvalidParametersFault{};
    }

    template <typename T>
    static void loadValue(std::optional<T> &dst, const jsonrpc::Value &src) {
      if (!src.IsNil()) {
        T t;
        loadValue(t, src);
        dst = std::move(t);
      } else {
        dst = std::nullopt;
      }
    }

    template <typename SequenceContainer,
              typename = typename SequenceContainer::value_type,
              typename = typename SequenceContainer::iterator>
    static void loadValue(SequenceContainer &dst, const jsonrpc::Value &src) {
      if (!src.IsNil() && src.IsArray()) {
        for (auto &v : src.AsArray()) {
          typename SequenceContainer::value_type t;
          loadValue(t, v);
          dst.insert(dst.end(), std::move(t));
        }
      } else {
        throwInvalidType();
      }
    }

    template <typename T>
    static std::enable_if_t<std::is_integral_v<T>, void> loadValue(
        T &dst, const jsonrpc::Value &src) {
      if (not src.IsInteger32() and not src.IsInteger64()) {
        throwInvalidType();
      }
      auto num = src.AsInteger64();
      if constexpr (std::is_signed_v<T>) {
        if (num < std::numeric_limits<T>::min()
            or num > std::numeric_limits<T>::max()) {
          throwInvalidValue();
        }
      } else {
        if (num < 0
            or static_cast<uint64_t>(num) > std::numeric_limits<T>::max()) {
          throwInvalidValue();
        }
      }
      dst = num;
    }

    static void loadValue(bool &dst, const jsonrpc::Value &src) {
      if (not src.IsBoolean()) {
        throwInvalidType();
      }
      dst = src.AsBoolean();
    }

    static void loadValue(std::string &dst, const jsonrpc::Value &src) {
      if (not src.IsString()) {
        throwInvalidType();
      }
      dst = src.AsString();
    }

    template <size_t N>
    static void loadValue(common::Blob<N> &out, const jsonrpc::Value &j) {
      auto &s = j.AsString();
      if (s.starts_with("0x")) {
        out = unwrap(out.fromHexWithPrefix(s));
      } else {
        out = unwrap(out.fromHex(s));
      }
    }

    static void loadValue(common::Buffer &out, const jsonrpc::Value &j) {
      auto &s = j.AsString();
      if (s.starts_with("0x")) {
        out = unwrap(common::unhexWith0x(s));
      } else {
        out = unwrap(common::unhex(s));
      }
    }

    static void loadValue(primitives::MmrLeavesProof &out,
                          const jsonrpc::Value &j) {
      loadValue(out.block_hash, mapAt(j, "blockHash"));
      loadValue(out.leaves, mapAt(j, "leaves"));
      loadValue(out.proof, mapAt(j, "proof"));
    }
  };

  template <size_t I, typename... T>
  void decodeArgsLoop(std::tuple<T...> &args,
                      const jsonrpc::Request::Parameters &json) {
    if constexpr (I < sizeof...(T)) {
      static const jsonrpc::Value kNull;
      LoadValue::loadValue(std::get<I>(args),
                           I < json.size() ? json.at(I) : kNull);
      decodeArgsLoop<I + 1>(args, json);
    }
  }

  template <typename... T>
  void decodeArgs(std::tuple<T...> &args,
                  const jsonrpc::Request::Parameters &json) {
    if (json.size() > sizeof...(T)) {
      throw jsonrpc::InvalidParametersFault("Incorrect number of params");
    }
    decodeArgsLoop<0>(args, json);
  }
}  // namespace kagome::api::details
