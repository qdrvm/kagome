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
#include "scale_error.hpp"

#include <variant>

using kagome::common::Buffer;

namespace kagome::common::scale::variant {
  namespace detail {
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
    struct VariantEncoder {
      using Variant = std::variant<T...>;
      using Result = outcome::result<void>;
      const Variant &v;
      Result &res;
      Buffer &out;

      VariantEncoder(const Variant &v, Buffer &buf, Result &res)
          : v{v}, out{buf}, res{res} {}

      template <class H>
      void apply(uint8_t index) {
        // if type matches alternative in variant then encode
        if (std::holds_alternative<H>(v)) {
          // first byte means type index
          out.putUint8(index);
          // get alternative ptr
          auto *alternative = std::get_if<H>(&v);
          if (nullptr == alternative) {
            res = EncodeError::kNoAlternative;
            return;
          }
          // encode the value using custom type encoder
          auto &&encode_result = TypeEncoder<H>{}.encode(*alternative, out);
          if (!encode_result) {
            res = encode_result.error();
          }
        }
      }
    };

    template <class... T>
    struct VariantDecoder {
      using Variant = std::variant<T...>;
      using Result = outcome::result<Variant>;
      Result &r;
      Stream &s;
      uint8_t target_type_index;
      static constexpr uint8_t types_count = sizeof...(T);

      VariantDecoder(uint8_t target_type_index, Result &r, Stream &s)
          : target_type_index{target_type_index}, r{r}, s{s} {};

      template <class H>
      void apply(uint8_t index) {
        if (target_type_index >= types_count) {
          r = DecodeError::kWrongTypeIndex;
        } else if (index == target_type_index) {
          // decode custom type
          auto &&res = TypeDecoder<H>{}.decode(s);
          if (!res) {
            r = res.error();
          } else {
            r = Variant{res.value()};
          }
        }
      }
    };

    template <class F, class... T>
    void for_each_apply(F &f) {
      detail::for_each_apply_impl<0, F, T...>(f);
    }
  };  // namespace detail

  /**
   * @brief encodes std::variant value
   * @tparam T... sequence of types
   * @param v variant value
   * @param out output buffer
   */
  template <class... T>
  outcome::result<void> encodeVariant(const std::variant<T...> &v,
                                      Buffer &out) {
    outcome::result<void> res = outcome::success();
    auto encoder = detail::VariantEncoder<T...>(v, out, res);
    detail::for_each_apply<decltype(encoder), T...>(encoder);
    return res;
  }

  /**
   * @brief decodes variant value
   * @tparam T... sequence of types
   * @param stream source stream
   * @return decoded value or error
   */
  template <class... T>
  outcome::result<std::variant<T...>> decodeVariant(Stream &stream) {
    // first byte means type index
    OUTCOME_TRY(type_index, fixedwidth::decodeUint8(stream));

    outcome::result<std::variant<T...>> result = outcome::success();
    auto decoder = detail::VariantDecoder<T...>(type_index, result, stream);

    detail::for_each_apply<decltype(decoder), T...>(decoder);

    return result;
  }

}  // namespace kagome::common::scale::variant

#endif  // KAGOME_SCALE_VARIANT_HPP
