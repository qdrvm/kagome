/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EVENT_EMITTER_HPP
#define KAGOME_EVENT_EMITTER_HPP

#include <functional>

#include <boost/signals2.hpp>

namespace kagome::common {
  template <typename Tag, typename... EventArgs>
  class EventEmitter {
   protected:
    using EventHandler = void(EventArgs...);
    using EventSignal = boost::signals2::signal<EventHandler>;

    static void subscribe(const std::function<EventHandler> &handler) {
      signal_.connect(handler);
    }

    static void fire(EventArgs... args) {
      signal_(std::forward<EventArgs...>(args...));
    }

   private:
    static EventSignal signal_;
  };

  template <typename Tag, typename... EventArgs>
  typename EventEmitter<Tag, EventArgs...>::EventSignal
      EventEmitter<Tag, EventArgs...>::signal_{};

}  // namespace kagome::common

#endif  // KAGOME_EVENT_EMITTER_HPP
