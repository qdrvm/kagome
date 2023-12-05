/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/jrpc/decode_args.hpp"
#include "api/jrpc/value_converter.hpp"
#include "outcome/into.hpp"
#include "utils/std_function_args.hpp"
#include "utils/std_tuple_skip_first.hpp"

namespace kagome::api {
  jsonrpc::Value jrpcFnResult(auto &&call) {
    using R = std::invoke_result_t<decltype(call)>;
    if constexpr (not std::is_same_v<R, void>) {
      auto result = outcome::into(call());
      if (result) {
        if constexpr (not std::is_same_v<decltype(result.value()), void>) {
          return makeValue(result.value());
        }
        return {};
      }
      throw jsonrpc::Fault(result.error().message());
    } else {
      call();
      return {};
    }
  }

  auto jrpcFn(auto fn) {
    return [fn{std::move(fn)}](
               const jsonrpc::Request::Parameters &params) -> jsonrpc::Value {
      StdFunctionArgs<decltype(std::function{fn})> args;
      details::decodeArgs(args, params);
      return jrpcFnResult([&] { return std::apply(fn, std::move(args)); });
    };
  }

  template <typename T>
  auto jrpcFn(std::enable_shared_from_this<T> *self, auto fn) {
    return [weak{self->weak_from_this()}, fn{std::move(fn)}](
               const jsonrpc::Request::Parameters &params) -> jsonrpc::Value {
      auto self = weak.lock();
      if (not self) {
        throw jsonrpc::InternalErrorFault("weak_ptr expired");
      }
      StdTupleSkipFirst<StdFunctionArgs<decltype(std::function{fn})>> args;
      details::decodeArgs(args, params);
      return jrpcFnResult([&] {
        return std::apply(
            fn,
            std::tuple_cat(std::make_tuple(std::move(self)), std::move(args)));
      });
    };
  }
}  // namespace kagome::api
