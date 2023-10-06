/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_BIND_BY_LAMBDA_HPP
#define KAGOME_CORE_INJECTOR_BIND_BY_LAMBDA_HPP

#include <boost/di/extension/scopes/shared.hpp>

namespace kagome::injector {
  /**
   * Creates single instance per injector.
   *
   * `boost::di::bind<T>.to(callable)` is hardcoded to
   * `boost::di::scopes::instance` and prevents scope redefinition with
   * `.in(scope)`.
   *
   * `callable` will be called more than once. Caching result with `static` will
   * behave like `boost::di::scopes::singleton`.
   *
   * `BindByLambda` wraps `boost::di::extension::shared` to create single
   * instance per injector, and adds `callable` support from
   * `boost::di::scopes::instance`.
   */
  struct BindByLambda {
    template <typename F, typename Provider>
    struct ProviderF {
      auto get() const {
        return f(provider.super());
      }
      F &f;
      const Provider &provider;
    };

    template <typename T, typename F>
    struct scope {
      explicit scope(const F &f) : f_{f} {}

      template <typename, typename>
      using is_referable = std::true_type;

      template <typename, typename, typename Provider>
      static boost::di::wrappers::shared<boost::di::extension::detail::shared,
                                         T>
      try_create(const Provider &provider);

      template <typename, typename, typename Provider>
      auto create(const Provider &provider) & {
        return base_.template create<void, void>(
            ProviderF<F, Provider>{f_, provider});
      }

      template <typename, typename, typename Provider>
      auto create(const Provider &provider) && {
        return std::move(base_).template create<void, void>(
            ProviderF<F, Provider>{f_, provider});
      }

      boost::di::extension::detail::shared::scope<void, T> base_;
      F f_;
    };
  };

  template <typename T, typename F>
  auto bind_by_lambda(const F &f) {
    return boost::di::core::dependency<BindByLambda, T, F>{f};
  }
}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_BIND_BY_LAMBDA_HPP
