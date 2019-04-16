/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RESULT_HPP
#define KAGOME_RESULT_HPP

#include <boost/variant.hpp>

#include "common/visitor.hpp"
#include "macro/nodiscard.hpp"

/*
 * Result is a type which represents value or an error, and values and errors
 * are template parametrized. Working with value wrapped in result is mostly
 * done using match() function, which accepts 2 functions: for value and error
 * cases. There are accessor methods which return an std::optional with either
 * Value or Error, but their usage is recommended to be avoided in favour of
 * match() function.
 */

namespace kagome::expected {

  /*
   * Value and error types can be constructed from any value or error, if
   * underlying types are constructible. Example:
   *
   * @code
   * Value<std::string> v = Value<const char *>("hello");
   * @nocode
   */

  template <typename T>
  struct Value {
    T value;
    template <typename V>
    operator Value<V>() {  // NOLINT
      return {value};
    }
  };

  template <>
  struct Value<void> {};

  template <typename T>
  Value(T)->Value<T>;

  template <typename E>
  struct Error {
    E error;
    template <typename V>
    operator Error<V>() { // NOLINT
      return {error};
    }
  };

  template <>
  struct Error<void> {};

  template <typename T>
  Error(T)->Error<T>;

  /**
   * In case of an attempt to extract a Value or an Error from the Result,
   * when the one is not present, an exception of one of two following types
   * is thrown. UnwrapException is a common ancestor for them.
   */
  class UnwrapException : public std::exception {
    NODISCARD const char *what() const noexcept override = 0;
  };

  class NoValueException : public UnwrapException {
    NODISCARD const char *what() const noexcept override {
      return "No Value stored in the Result; Check Error";
    }
  };

  class NoErrorException : public UnwrapException {
    NODISCARD const char *what() const noexcept override {
      return "No Error stored in the Result; Check Value";
    }
  };

  /**
   * Result is a specialization of a variant type with value or error
   * semantics.
   * @tparam V type of value
   * @tparam E error type
   */
  template <typename V, typename E>
  class Result : public boost::variant<Value<V>, Error<E>> {
    using variant_type = boost::variant<Value<V>, Error<E>>;
    using variant_type::variant_type;  // inherit constructors

   public:
    using ValueType = Value<V>;
    using ErrorType = Error<E>;

    Result() = delete;

    /**
     * match is a function which allows working with result's underlying
     * types, you must provide 2 functions to cover success and failure cases.
     * Return type of both functions must be the same. Example usage:
     * @code
     * result.match([](Value<int> v) { std::cout << v.value; },
     *              [](Error<std::string> e) { std::cout << e.error; });
     * @nocode
     */
    template <typename ValueMatch, typename ErrorMatch>
    constexpr auto match(ValueMatch &&value_func, ErrorMatch &&error_func) {
      return visit_in_place(*this, std::forward<ValueMatch>(value_func),
                            std::forward<ErrorMatch>(error_func));
    }

    /**
     * Const alternative for match function
     */
    template <typename ValueMatch, typename ErrorMatch>
    constexpr auto match(ValueMatch &&value_func,
                         ErrorMatch &&error_func) const {
      return visit_in_place(*this, std::forward<ValueMatch>(value_func),
                            std::forward<ErrorMatch>(error_func));
    }

    /**
     * This method returns a reference to the value,
     * contained in the Result, if it contains value,
     * or throws a NoValueException exception otherwise.
     */
    V &getValueRef() {
      return match(
          [](Value<V> &v) -> std::reference_wrapper<V> { return v.value; },
          [](const Error<E> &_) -> std::reference_wrapper<V> {
            throw NoValueException();
          });
    }

    /**
     * This method returns a reference to the error,
     * contained in the Result, if it contains error,
     * or throws a NoErrorException exception otherwise.
     */
    E &getErrorRef() {
      return match(
          [](Error<E> &e) -> std::reference_wrapper<E> { return e.error; },
          [](const Value<V> &_) -> std::reference_wrapper<E> {
            throw NoErrorException();
          });
    }

    /**
     * This method returns a reference to the value,
     * contained in the Result, if it contains value,
     * or throws a NoValueException exception otherwise.
     */
    NODISCARD constexpr const V &getValueRef() const {
      return match(
          [](Value<V> &v) -> std::reference_wrapper<V> { return v.value; },
          [](const Error<E> &_) -> std::reference_wrapper<V> {
            throw NoValueException();
          });
    }

    /**
     * This method returns a reference to the error,
     * contained in the Result, if it contains error,
     * or throws a NoErrorException exception otherwise.
     */
    NODISCARD constexpr const E &getErrorRef() const {
      return match(
          [](Error<E> &e) -> std::reference_wrapper<E> { return e.error; },
          [](const Value<V> &_) -> std::reference_wrapper<E> {
            throw NoErrorException();
          });
    }

    /**
     * This method moves the value contained in the Result, if it was stored,
     * or throws a NoValueException exception otherwise.
     */
    NODISCARD constexpr V getValue() {
      return match([](Value<V> &v) -> V { return std::forward<V>(v.value); },
                   [](const Error<E> &_) -> V { throw NoValueException(); });
    }

    /**
     * This method moves the error contained in the Result, if
     * it was stored, or throws a NoErrorException exception otherwise.
     */
    NODISCARD constexpr E getError() {
      return match([](Error<E> &e) -> E { return std::forward<E>(e.error); },
                   [](const Value<V> &_) -> E { throw NoErrorException(); });
    }

    /**
     * Returns true if the result contains a value, else returns false
     */
    NODISCARD constexpr bool hasValue() const noexcept {
      return match([](const Value<V> &v) { return true; },
                   [](const Error<E> &_) { return false; });
    }

    /**
     * Returns true if the result contains an error, else returns false
     */
    NODISCARD constexpr bool hasError() const noexcept {
      return match([](const Value<V> &v) { return false; },
                   [](const Error<E> &_) { return true; });
    }

    /**
     * Lazy error AND-chaining
     * Works by the following table (aka boolean lazy AND):
     * err1 * any  -> err1
     * val1 * err2 -> err2
     * val1 * val2 -> val2
     *
     * @param new_res second chain argument
     * @return new_res if this Result contains a value
     *         otherwise return this
     */
    template <typename Value>
    NODISCARD constexpr Result<Value, E> and_res(const Result<Value, E> &new_res) const
        noexcept {
      return visit_in_place(
          *this, [res = new_res](ValueType v) { return res; },
          [](ErrorType err) -> Result<Value, E> { return err; });
    }

    /**
     * Lazy error OR-chaining
     * Works by the following table (aka boolean lazy OR):
     * val1 * any  -> val1
     * err1 * val2 -> val2
     * err1 * err2 -> err2
     *
     * @param new_res second chain argument
     * @return new_res if this Result contains a error
     *         otherwise return this
     */
    template <typename Value>
    NODISCARD constexpr Result<Value, E> or_res(const Result<Value, E> &new_res) const
        noexcept {
      return visit_in_place(
          *this, [](ValueType val) -> Result<Value, E> { return val; },
          [res = new_res](ErrorType e) { return res; });
    }
  };

  template <typename ResultType>
  using ValueOf = typename ResultType::ValueType;
  template <typename ResultType>
  using ErrorOf = typename ResultType::ErrorType;

  /**
   * Get a new result with the copied value or mapped error
   * @param res base Result for getting new one
   * @param map callback for error mapping
   * @return result with changed error
   */
  template <typename Err1, typename Err2, typename V, typename Fn>
  Result<V, Err1> map_error(const Result<V, Err2> &res, Fn &&map) noexcept {
    return visit_in_place(
        res, [](Value<V> val) -> Result<V, Err1> { return val; },
        [map](Error<Err2> err) -> Result<V, Err1> {
          return Error<Err1>{map(err.error)};
        });
  }

  /**
   * Bind operator allows chaining several functions which return result. If
   * result contains error, it returns this error, if it contains value,
   * function f is called.
   * @param f function which return type must be compatible with original
   * result
   */
  template <typename T, typename E, typename Transform>
  constexpr auto operator|(Result<T, E> r, Transform &&f) ->
      typename std::enable_if<
          not std::is_same<decltype(f(std::declval<T>())), void>::value,
          decltype(f(std::declval<T>()))>::type {
    using return_type = decltype(f(std::declval<T>()));
    return r.match([&f](const Value<T> &v) { return f(v.value); },
                   [](const Error<E> &e) { return return_type(e); });
  }

  /**
   * Bind operator overload for functions which do not accept anything as a
   * parameter. Allows execution of a sequence of unrelated functions, given
   * that all of them return Result
   * @param f function which accepts no parameters and returns result
   */
  template <typename T, typename E, typename Procedure>
  constexpr auto operator|(Result<T, E> r, Procedure f) ->
      typename std::enable_if<not std::is_same<decltype(f()), void>::value,
                              decltype(f())>::type {
    using return_type = decltype(f());
    return r.match([&f](const Value<T> &v) { return f(); },
                   [](const Error<E> &e) { return return_type(e); });
  }

  /**
   * Polymorphic Result is simple alias for result type, which can be used to
   * work with polymorphic objects. It is achieved by wrapping V and E in a
   * polymorphic container (std::shared_ptr is used by default). This
   * simplifies declaration of polymorphic result.
   *
   * Note: ordinary result itself stores both V and E directly inside itself
   * (on the stack), polymorphic result stores objects wherever VContainer and
   * EContainer store them, but since you need polymorphic behavior, it will
   * probably be on the heap. That is why polymorphic result is generally
   * slower, and should be used ONLY when polymorphic behaviour is required,
   * hence the name. For all other use cases, stick to basic Result
   */
  template <typename V, typename E, typename VContainer = std::shared_ptr<V>,
            typename EContainer = std::shared_ptr<E>>
  using PolymorphicResult = Result<VContainer, EContainer>;

}  // namespace kagome::expected

#endif  // KAGOME_RESULT_HPP
