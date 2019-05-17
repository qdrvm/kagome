/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EMITTER_HPP
#define KAGOME_EMITTER_HPP

#include <functional>

#include <boost/signals2.hpp>

namespace libp2p::event {

  template <typename Tag, typename... EventArgs>
  struct Emitter {
    template <typename _Tag, typename... _EventArgs,
              typename = std::enable_if_t<std::is_same<_Tag, Tag>::value>,
              typename = std::enable_if_t<
                  std::is_same<_EventArgs..., EventArgs...>::value>>
    void on(const std::function<void(_EventArgs...)> &handler) {
      signal_.connect(handler);
    }

    template <typename _Tag, typename... _EventArgs,
              typename = std::enable_if_t<std::is_same<_Tag, Tag>::value>,
              typename = std::enable_if_t<
                  std::is_same<_EventArgs..., EventArgs...>::value>>
    void emit(_EventArgs &&... args) {
      signal_(std::forward<_EventArgs &&...>(args...));
    }

   private:
    boost::signals2::signal<void(EventArgs...)> signal_;
  };

#define USINGS(args...)    \
  using Emitter<args>::on; \
  using Emitter<args>::emit;

}  // namespace libp2p::event

#endif  // KAGOME_EMITTER_HPP
