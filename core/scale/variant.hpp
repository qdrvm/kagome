/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_VARIANT_HPP
#define KAGOME_SCALE_VARIANT_HPP

#include <boost/variant.hpp>
#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "common/visitor.hpp"
#include "scale/compact.hpp"
#include "scale/fixedwidth.hpp"
#include "scale/scale_error.hpp"
#include "scale/type_decoder.hpp"
#include "scale/type_encoder.hpp"

namespace kagome::scale::variant {
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

    // TODO(yuraz): refactor PRE-119
    template <class... T>
    struct VariantEncoder {
      using Variant = boost::variant<T...>;
      using Result = outcome::result<void>;
      const Variant &v_;
      kagome::common::Buffer &out_;
      Result &res_;

      VariantEncoder(const Variant &v, kagome::common::Buffer &buf, Result &res)
          : v_{v}, out_{buf}, res_{res} {}

      template <class H>
      void apply(uint8_t index) {
        // if type matches alternative in variant then encode
        kagome::visit_in_place(
            v_,
            [this, index](const H &h) {
              // first byte means type index
              out_.putUint8(index);
              auto &&encode_result = TypeEncoder<H>{}.encode(h, out_);
              if (!encode_result) {
                res_ = encode_result.error();
              }
            },
            [](const auto & /*unused*/) {});
      }
    };

    // TODO(yuraz): refactor PRE-119
    template <class... T>
    struct VariantDecoder {
      using Variant = boost::variant<T...>;
      using Result = outcome::result<Variant>;
      uint8_t target_type_index_;
      Result &r_;
      common::ByteStream &s_;
      static constexpr uint8_t types_count = sizeof...(T);

      VariantDecoder(uint8_t target_type_index, Result &r,
                     common::ByteStream &s)
          : target_type_index_{target_type_index}, r_{r}, s_{s} {};

      template <class H>
      void apply(uint8_t index) {
        if (target_type_index_ >= types_count) {
          r_ = DecodeError::WRONG_TYPE_INDEX;
        } else if (index == target_type_index_) {
          // decode custom type
          auto &&res = TypeDecoder<H>{}.decode(s_);
          if (!res) {
            r_ = res.error();
          } else {
            r_ = Variant{static_cast<H>(res.value())};
          }
        }
      }
    };

    template <class F, class... T>
    void for_each_apply(F &f) {
      detail::for_each_apply_impl<0, F, T...>(f);
    }
  }  // namespace detail

  /**
   * @brief encodes boost::variant value
   * @tparam T... sequence of types
   * @param v variant value
   * @param out output buffer
   */
  template <class... T>
  outcome::result<void> encodeVariant(const boost::variant<T...> &v,
                                      kagome::common::Buffer &out) {
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
  outcome::result<boost::variant<T...>> decodeVariant(
      common::ByteStream &stream) {
    // first byte means type index
    OUTCOME_TRY(type_index, fixedwidth::decodeUint8(stream));

    outcome::result<boost::variant<T...>> result = outcome::success();
    auto decoder = detail::VariantDecoder<T...>(type_index, result, stream);

    detail::for_each_apply<decltype(decoder), T...>(decoder);

    return result;
  }

}  // namespace kagome::scale::variant

#endif  // KAGOME_SCALE_VARIANT_HPP
