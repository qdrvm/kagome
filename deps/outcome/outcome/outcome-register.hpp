/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OUTCOME_REGISTER_HPP
#define KAGOME_OUTCOME_REGISTER_HPP

#include <boost/config.hpp>  // for BOOST_SYMBOL_EXPORT
#include <boost/preprocessor.hpp>
#include <string>
#include <system_error>  // bring in std::error_code et al

#ifndef KAGOME_EXPORT
#if defined(BOOST_SYMBOL_EXPORT)
#define KAGOME_EXPORT BOOST_SYMBOL_EXPORT
#else
#define KAGOME_EXPORT extern inline
#endif
#endif

#define STRINGIFY(x) #x

#define __FILLER_0(X, Y) ((X, Y)) __FILLER_1
#define __FILLER_1(X, Y) ((X, Y)) __FILLER_0
#define __FILLER_0_END
#define __FILLER_1_END

#define __CASE_P(_cond, _ret) \
  case _cond:                 \
    return _ret;

#define __CASE(R, _, _tuple)                    \
  __CASE_P((BOOST_PP_TUPLE_ELEM(2, 0, _tuple)), \
           (BOOST_PP_TUPLE_ELEM(2, 1, _tuple)))

#define __WRITE_CASES(_seq) \
  BOOST_PP_SEQ_FOR_EACH(__CASE, _, BOOST_PP_CAT(__FILLER_0 _seq, _END))

#define __REGISTER_STRUCT(NAME)                          \
  namespace std {                                        \
    template <>                                          \
    struct is_error_code_enum<NAME> : std::true_type {}; \
  }

#define __WRITE_MESSAGE_FUNCTION(NAME, _seq)        \
  std::string message(int c) const override final { \
    switch (static_cast<NAME>(c)) {                 \
      __WRITE_CASES(_seq)                           \
      default:                                      \
        return "unknown";                           \
    }                                               \
  }

#define __REGISTER_CATEGORY(NAME, _seq)                            \
  namespace detail {                                               \
    class NAME##_category : public std::error_category {           \
     public:                                                       \
      const char *name() const noexcept final {                    \
        return STRINGIFY(NAME);                                    \
      }                                                            \
      __WRITE_MESSAGE_FUNCTION(NAME, _seq)                         \
    }; /* end of class */                                          \
  }    /* end of namespace */                                      \
                                                                   \
  KAGOME_EXPORT const detail::NAME##_category &NAME##_category() { \
    static detail::NAME##_category c;                              \
    return c;                                                      \
  }                                                                \
                                                                   \
  inline std::error_code make_error_code(NAME e) {                 \
    return {static_cast<int>(e), NAME##_category()};               \
  }

#define OUTCOME_REGISTER_ERROR(Name, _seq) \
  __REGISTER_STRUCT(Name)                  \
  __REGISTER_CATEGORY(Name, _seq)

// BOOST_PP_TUPLE_ELEM

#endif  // KAGOME_OUTCOME_REGISTER_HPP
