/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_GENERIC_ITERATOR_HPP
#define KAGOME_GENERIC_ITERATOR_HPP

namespace kagome::face {

  template <typename Container>
  class GenericIterator {
   public:
    using value_type = typename Container::value_type;

    virtual ~GenericIterator() = default;

    virtual value_type *get() = 0;
    virtual value_type const *get() const = 0;

    virtual value_type &operator*() = 0;
    virtual value_type const &operator*() const = 0;

    virtual GenericIterator<Container> &operator++() = 0;

    value_type &operator->() {
      return **this;
    }

    virtual bool operator!=(const GenericIterator<Container> &other) const {
      return get() != other.get();
    }

    bool operator==(const GenericIterator<Container> &other) {
      return get() == other.get();
    }

  };

}  // namespace kagome::face

#endif  // KAGOME_GENERIC_ITERATOR_HPP
