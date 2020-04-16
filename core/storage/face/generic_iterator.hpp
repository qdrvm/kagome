/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_GENERIC_ITERATOR_HPP
#define KAGOME_GENERIC_ITERATOR_HPP

#include <memory>

namespace kagome::face {

  /**
   * An interface for an iterator
   * @tparam Container over which the iterator would iterate
   */
  template <typename Container>
  class GenericIterator {
   public:
    using value_type = typename Container::value_type;

    virtual ~GenericIterator() = default;

    // needed as there's no simple way to copy an object by a pointer to its
    // abstract interface
    virtual std::unique_ptr<GenericIterator> clone() const = 0;

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
