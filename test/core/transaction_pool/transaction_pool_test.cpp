/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "transaction_pool/impl/transaction_pool_impl.hpp"

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"
#include "testutil/literals.hpp"

using kagome::face::ForwardIterator;
using kagome::face::GenericIterator;
using kagome::face::GenericList;
using kagome::transaction_pool::TransactionPoolImpl;
using kagome::primitives::Transaction;
using kagome::primitives::TransactionTag;

template <typename T>
class It : public GenericIterator<GenericList<T>> {
  template <typename>
  friend class StdList;

 public:
  using value_type = T;

  explicit It(typename std::list<T>::iterator l) : it_{l} {}
  ~It() = default;

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
class StdList : public GenericList<T> {
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
    auto &begin_it = dynamic_cast<It<T> const&>(begin.get_iterator());
    auto &end_it = dynamic_cast<It<T> const&>(end.get_iterator());
    list_.erase(begin_it.it_, end_it.it_);
  }

  ForwardIterator<GenericList<T>> begin() override {
    return {std::make_shared<It<T>>(list_.begin())};
  }
  ForwardIterator<GenericList<T>> end() override {
    return {std::make_shared<It<T>>(list_.end())};
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



TEST(TransactionPoolTest, Create) {
  auto tp = std::make_unique<TransactionPoolImpl>(
      std::make_unique<StdList<Transaction>>(),
      std::make_unique<StdList<Transaction>>());

  Transaction t1, t2, t3;
  t1.provides = {"1234"_unhex};
  t2.provides = {"abcd"_unhex};
  t3.requires = {"1234"_unhex, "abcd"_unhex};

  EXPECT_OUTCOME_TRUE_1(tp->submit({t1, t3}));
  EXPECT_EQ(tp->getStatus().waiting(), 1);
  ASSERT_EQ(tp->getStatus().ready(), 1);
  EXPECT_OUTCOME_TRUE_1(tp->submit({t2}));
  EXPECT_EQ(tp->getStatus().waiting(), 0);
  ASSERT_EQ(tp->getStatus().ready(), 3);
}
