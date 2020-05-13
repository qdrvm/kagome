/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_DETAIL_TUPLE_HPP
#define KAGOME_SCALE_DETAIL_TUPLE_HPP

#include <boost/variant.hpp>
#include <tuple>

namespace kagome::scale::detail {
  namespace tuple_impl {
    template <class F, uint8_t index>
    void apply_once(F &f) {
      f.template apply<index>();
    }

    template <uint8_t i, class F, class Head, class... T>
    void for_each_apply_impl(F &f) {
      apply_once<F, i>(f);
      for_each_apply_impl<i + 1, F, T...>(f);
    }

    template <uint8_t i, class F>
    void for_each_apply_impl(F &f) {
      // do nothing, end of recursion
    }

    template <class Stream, class... T>
    class TupleEncoder {
     public:
      using tuple_type_t = std::tuple<T...>;
      TupleEncoder(const tuple_type_t &v, Stream &s) : v_{v}, s_{s} {}

      template <uint8_t index>
      void apply() {
        s_ << std::get<index>(v_);
      }

     private:
      const tuple_type_t &v_;
      Stream &s_;
    };

    template <class Stream, class... T>
    class TupleDecoder {
     public:
      using tuple_type_t = std::tuple<T...>;
      TupleDecoder(tuple_type_t &v, Stream &s) : v_{v}, s_{s} {}

      template <uint8_t index>
      void apply() {
        s_ >> std::get<index>(v_);
      }

     private:
      tuple_type_t &v_;
      Stream &s_;
    };

    template <class F, class... T>
    void for_each_apply(F &f) {
      tuple_impl::for_each_apply_impl<0, F, T...>(f);
    }
  }  // namespace tuple_impl

  /**
   * @brief encodes std::tuple value
   * @tparam Stream output stream type
   * @tparam T... sequence of types of the tuple
   * @param v variant value
   * @param s encoder stream
   */
  template <class Stream, class... T>
  Stream &encodeTuple(const std::tuple<T...> &v, Stream &s) {
    auto &&encoder = tuple_impl::TupleEncoder(v, s);
    tuple_impl::for_each_apply<decltype(encoder), T...>(encoder);
    return s;
  }

  /**
   * @brief decodes std::tuple value
   * @tparam Stream input stream type
   * @tparam T... sequence of types of the tuple
   * @param stream source stream
   * @return decoded value or error
   */
  template <class Stream, class... T>
  Stream &decodeTuple(Stream &stream, std::tuple<T...> &result) {
    auto &&decoder = tuple_impl::TupleDecoder(result, stream);
    tuple_impl::for_each_apply<decltype(decoder), T...>(decoder);

    return stream;
  }

}  // namespace kagome::scale::detail

#endif  // KAGOME_SCALE_DETAIL_TUPLE_HPP
