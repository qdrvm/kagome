/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_VARIANT_HPP
#define KAGOME_SCALE_VARIANT_HPP

#include "common/buffer.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/type_decoder.hpp"
#include "scale/type_encoder.hpp"

#include <outcome/outcome.hpp>

#include <variant>

namespace kagome::common::scale::variant {
  namespace detail {
    template <uint8_t i>
    struct Counter {
      static const uint8_t value = i;
    };

    template <uint8_t i, class F, class H, class... T>
    void for_each_apply_impl(F &f);

    template <class F, class H>
    void apply_once(F &f, uint8_t i) {
      f.template apply<H>(i);
    }

    template <uint8_t i, class F>
    void for_each_apply_impl(F &f) {
      // do nothing, end of recursion
    }

    template <uint8_t i, class F, class H, class... T>
    void for_each_apply_impl(F &f) {
      apply_once<F, H>(f, i);
      for_each_apply_impl<i + 1, F, T...>(f);
    }

    template <class... T>
    struct EncodeFunctor {
      using V = std::variant<T...>;
      const V &v;
      Buffer &out;

      EncodeFunctor(const V &v, Buffer &buf) : v{v}, out{buf} {}

      template <class H>
      void apply(uint8_t index) {
        if (std::holds_alternative<H>(v)) {
          out.putUint8(index);
          TypeEncoder<H>{}.encode(std::get<H>(v), out);
        }
      }
    };

    template <class F, class... T>
    void for_each_apply(F &f) {
      for_each_apply_impl<0, F, T...>(f);
    }
  }  // namespace detail

  template <class... T>
  void encodeVariant(const std::variant<T...> &v, Buffer &out) {
    auto functor = detail::EncodeFunctor<T...>(v, out);
    detail::for_each_apply<decltype(functor), T...>(functor);
  }

  template <class... T>
  outcome::result<std::variant<T...>> decodeVariant(Stream &stream) {
    std::terminate();  // not implemented yet
    return outcome::failure(1);
  }

}  // namespace kagome::common::scale::variant

#endif  // KAGOME_SCALE_VARIANT_HPP
