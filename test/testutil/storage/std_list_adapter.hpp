/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STD_LIST_ADAPTER_HPP
#define KAGOME_STD_LIST_ADAPTER_HPP

#include "storage/face/generic_list.hpp"
#include "storage/face/generic_iterator.hpp"

namespace test {

  using kagome::face::GenericIterator;
  using kagome::face::GenericList;
  using kagome::face::ForwardIterator;

  template <typename T>
  class StdListIterator : public GenericIterator<GenericList<T>> {
    template <typename>
    friend class StdListAdapter;

   public:
    using value_type = T;

    explicit StdListIterator(typename std::list<T>::iterator l) : it_{std::move(l)} {}
    ~StdListIterator() override = default;

    std::unique_ptr<GenericIterator<GenericList<T>>> clone()
        const override {
      return std::make_unique<StdListIterator<T>>(it_);
    }

    value_type *get() override {
      return &*it_;
    }

    value_type const *get() const override {
      return &*it_;
    }

    value_type &operator*() override {
      return *it_;
    }

    value_type const &operator*() const override {
      return *it_;
    }

    kagome::face::GenericIterator<GenericList<T>> &operator++() override {
      it_++;
      return *this;
    }

   private:
    typename std::list<T>::iterator it_;
  };

  template <typename T>
  class StdListAdapter: public GenericList<T> {
   public:
    using value_type = T;
    using size_type = typename GenericList<T>::size_type;
    using iterator = ForwardIterator<GenericList<T>>;

    void push_back(T &&t) override {
      list_.push_back(t);
    }
    void push_back(const T &t) override {
      list_.push_back(t);
    }
    void push_front(T &&t) override {
      list_.push_front(t);
    }
    void push_front(const T &t) override {
      list_.push_front(t);
    }
    T pop_back() override {
      value_type t = list_.back();
      list_.pop_back();
      return t;
    }
    T pop_front() override {
      value_type t = list_.front();
      list_.pop_front();
      return t;
    }

    void erase(const iterator &begin, const iterator &end) override {
      auto &begin_it =
          dynamic_cast<StdListIterator<T> const &>(begin.get_iterator());
      auto &end_it =
          dynamic_cast<StdListIterator<T> const &>(end.get_iterator());
      list_.erase(begin_it.it_, end_it.it_);
    }

    ForwardIterator<GenericList<T>> begin() override {
      return {std::make_unique<StdListIterator<T>>(list_.begin())};
    }
    ForwardIterator<GenericList<T>> end() override {
      return {std::make_unique<StdListIterator<T>>(list_.end())};
    }
    bool empty() const override {
      return list_.empty();
    }

    size_type size() const override {
      return list_.size();
    }

   private:
    std::list<T> list_;
  };

}

#endif //KAGOME_STD_LIST_ADAPTER_HPP
