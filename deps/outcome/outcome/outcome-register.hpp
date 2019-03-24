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

#define OUTCOME_USE_STD_IN_PLACE_TYPE 1

namespace __kagome {

  template <typename T>
  class Category : public std::error_category {
   public:
    const char *name() const noexcept final {
      return typeid(T).name();
    }

    std::string message(int c) const final {
      return toString(static_cast<T>(c));
    }

    static std::string toString(T t) {
      static_assert(!std::is_same<T, T>::value,
                    "toString<T>() was not specialised for the type T supplied");
      return "";
    }

    KAGOME_EXPORT static const Category<T> &get() {
      static const Category<T> c;
      return c;
    }
  }; /* end of class */

}  // namespace __kagome

/// MUST BE EXECUTED IN THE SAME NAMESPACE AS ENUM in cpp or hpp
#define OUTCOME_MAKE_ERROR_CODE(Enum)                              \
  inline std::error_code make_error_code(Enum e) {                 \
    return {static_cast<int>(e), __kagome::Category<Enum>::get()}; \
  }

/// MUST BE EXECUTED AT FILE LEVEL (no namespace) IN CPP
#define OUTCOME_REGISTER_CATEGORY(Enum, Name)            \
  namespace std {                                        \
    template <>                                          \
    struct is_error_code_enum<Enum> : std::true_type {}; \
  }                                                      \
                                                         \
  template <>                                            \
  std::string __kagome::Category<Enum>::toString(Enum Name)

#endif  // KAGOME_OUTCOME_REGISTER_HPP
