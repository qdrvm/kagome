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
#define UNIQUE_NAME(prefix) BOOST_PP_CAT(prefix, __LINE__)

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

#define __WRITE_MESSAGE_FUNCTION(ENUMTYPE, _seq)    \
  std::string message(int c) const override final { \
    switch (static_cast<ENUMTYPE>(c)) {             \
      __WRITE_CASES(_seq)                           \
      default:                                      \
        return "unknown";                           \
    }                                               \
  }

#define __REGISTER_CATEGORY(ENUMTYPE, NAME, _seq)            \
  class NAME : public std::error_category {                  \
   public:                                                   \
    NAME() = default;                                        \
    const char *name() const noexcept final {                \
      return STRINGIFY(ENUMTYPE);                            \
    }                                                        \
    __WRITE_MESSAGE_FUNCTION(ENUMTYPE, _seq)                 \
                                                             \
    KAGOME_EXPORT static const NAME &get() {                 \
      static const NAME c;                                   \
      return c;                                              \
    }                                                        \
                                                             \
  }; /* end of class */                                      \
                                                             \
  inline std::error_code make_error_code(const ENUMTYPE e) { \
    return {static_cast<int>(e), NAME::get()};               \
  }

/**
 * @brief Execute this macro in CPP-file OUTSIDE of any namespace to register
 * any enum as error code
 */
#define OUTCOME_REGISTER_ENUM(ENUMTYPE)                      \
  namespace std {                                            \
    template <>                                              \
    struct is_error_code_enum<ENUMTYPE> : std::true_type {}; \
  }

/**
 * @brief Execute this macro in CPP-file, in your namespace to register error
 * text
 */
#define OUTCOME_REGISTER_ERRORS(Enum, _seq) \
  __REGISTER_CATEGORY(Enum, UNIQUE_NAME(EnumErr), _seq)

// BOOST_PP_TUPLE_ELEM

#endif  // KAGOME_OUTCOME_REGISTER_HPP
