/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_COMMON_DYNAMIC_VARIANT_HPP
#define KAGOME_CORE_COMMON_DYNAMIC_VARIANT_HPP

#include <functional>
#include <type_traits>
#include <vector>

#include <boost/mpl/at.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/int.hpp>
#include <boost/variant.hpp>

namespace kagome::common {

  /// Impl details for boost::variant initialization by type index at runtime
  namespace dynamic_variant {
    /**
     * Initializes variant with default constructed object of some type
     * @tparam Concrete - a type to construct and assign to variant
     * @tparam Variant - type of variant variable
     * @param v - variant to intiialize
     */
    template <typename Concrete, typename Variant>
    void init_variant(Variant &v) {
      v = Concrete{};
    }

    /// Prepares initializers for each type of variant
    template <typename Variant>
    struct functors_vector_builder {
      std::vector<std::function<void(Variant &)>> *vec;
      template <typename Element>
      void operator()(Element) {
        vec->push_back(&init_variant<Element, Variant>);
      }
    };

    template <typename>
    struct is_boost_variant : std::false_type {};

    /**
     * Compile time checker that the type is boost::variant.
     *
     * Usage example:
     * ```
     * using MyVariant = boost::variant<int, std::string>;
     * ...
     * static_assert(is_boost_variant<MyVariant>::value);
     * ```
     *
     * @tparam Ts types held inside boost::variant
     */
    template <typename... Ts>
    struct is_boost_variant<boost::variant<Ts...>> : std::true_type {};
  }  // namespace dynamic_variant

  /**
   * Constructs a value inside boost::variant by type index from variant's
   * definition at runtime.
   *
   * Allows operating with variants as easy as converting an int to enum value
   * but with more complex types than in enums.
   *
   * No need to explicitly specify types below, compiler infers them from
   * constructor
   * @tparam Variant type defenition of boost::variant
   * @tparam Types types held inside Variant
   */
  template <typename Variant, typename Types = typename Variant::types>
  class VariantBuilder {
    Variant &v_;
    std::vector<std::function<void(Variant &)>> funcs_;

   public:
    /// @param v boost::variant variable to initialize
    explicit VariantBuilder(Variant &v) : v_(v) {
      static_assert(dynamic_variant::is_boost_variant<Variant>::value);
      dynamic_variant::functors_vector_builder<Variant> builder = {&funcs_};
      boost::mpl::for_each<Types>(builder);
    };

    /**
     * Initializes the referenced variant with default constructed instance of
     * object of type at \param index within \tparam Variant::types
     *
     * @param index index of type
     */
    void init(size_t index) {
      funcs_[index](v_);
    }
  };

}  // namespace kagome::common

#endif  // KAGOME_CORE_COMMON_DYNAMIC_VARIANT_HPP
