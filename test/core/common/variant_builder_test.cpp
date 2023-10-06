/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <variant>

#include <gtest/gtest.h>
#include "common/variant_builder.hpp"

using namespace kagome::common;

class A {};
class B {};

using Variant = boost::variant<A, B>;

/**
 * @given a boost::variant of types A or B
 * @when VariantBuilder used to initialize the variant with the first type
 * @then the object of correct type A is assigned into the variant
 */
TEST(VariantBuilder, Assign) {
  Variant v;
  VariantBuilder builder(v);
  builder.init(0);
  ASSERT_NO_THROW(boost::get<A>(v));
  ASSERT_THROW(boost::get<B>(v), boost::bad_get);
}

/**
 * @given a boost variant of types A or B initialized with type A
 * @when VariantBuilder is used to reinitialize it with the second type
 * @then the object of correct type B is assigned into the variant
 */
TEST(VariantBuilder, Reassign) {
  Variant v;
  VariantBuilder builder(v);
  builder.init(0);
  ASSERT_NO_THROW(boost::get<A>(v));
  ASSERT_THROW(boost::get<B>(v), boost::bad_get);

  builder.init(1);
  ASSERT_NO_THROW(boost::get<B>(v));
  ASSERT_THROW(boost::get<A>(v), boost::bad_get);
}
