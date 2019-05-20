/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_EMITTER_HPP
#define KAGOME_EMITTER_HPP

#include <functional>

#include <boost/signals2.hpp>

namespace libp2p::event {

  namespace detail {

    /**
     * Base template prototype
     */
    template <int EventsAmount, typename... Events>
    class EmitterBase;

    /**
     * "Main" recursive template of the Emitter
     * @tparam EventsAmount - how much events there are totally
     * @tparam ThisEvent - event, for which this Emitter template is responsible
     * @tparam OtherEvents - zero or more events, which go after (\param
     * ThisEvent)
     */
    template <int EventsAmount, typename ThisEvent, typename... OtherEvents>
    class EmitterBase<EventsAmount, ThisEvent, OtherEvents...>
        : public EmitterBase<EventsAmount, OtherEvents...> {
      /// type of underlying signal
      template <typename EventType>
      using Signal = boost::signals2::signal<void(EventType)>;

      /// base of this template - in fact, the base's "ThisEvent" is the the
      /// first of this "OtherEvents"
      using Base = EmitterBase<EventsAmount, OtherEvents...>;

     protected:
      /**
       * Get an underlying signal, if (\tparam ExpectedEvent) is the same as
       * ThisEvent type
       * @return reference to signal
       */
      template <typename ExpectedEvent>
      std::enable_if_t<std::is_same_v<ThisEvent, ExpectedEvent>,
                       Signal<ExpectedEvent>>
          &getSignal() {
        return signal_;
      }

      /**
       * SFINAE failure handler - continue the recursion, getting the base's
       * signal, if ThisEvent is not the same as (\tparam ExpectedEvent)
       * @return reference to signal
       */
      template <typename ExpectedEvent>
      std::enable_if_t<!std::is_same_v<ThisEvent, ExpectedEvent>,
                       Signal<ExpectedEvent>>
          &getSignal() {
        return Base::template getSignal<ExpectedEvent>();
      }

     private:
      /// the underlying signal
      Signal<ThisEvent> signal_;
    };

    /**
     * End of template recursion
     */
    template <int EventsAmount>
    class EmitterBase<EventsAmount> {
     protected:
      virtual ~EmitterBase() = default;
    };

  }  // namespace detail

  /**
   * Emitter, allowing to subscribe to events and emit them
   * @tparam Events, for which this Emitter is responsible; should be empty
   * structs or structs with desired params of the events
   */
  template <typename... Events>
  class Emitter : public detail::EmitterBase<sizeof...(Events), Events...> {
    /// underlying recursive base of the emitter
    using Base = detail::EmitterBase<sizeof...(Events), Events...>;

   public:
    /**
     * Subscribe to the specified event
     * @tparam Event, for which the subscription is to be registered
     * @param handler to be called, when a corresponding event is triggered
     */
    template <typename Event>
    void on(const std::function<void(Event)> &handler) {
      auto &signal = Base::template getSignal<Event>();
      signal.connect(handler);
    }

    /**
     * Trigger the specified event
     * @tparam Event, which is to be triggered
     * @param event - params (typically, a struct), with which the event is to
     * be triggered
     */
    template <typename Event>
    void emit(Event &&event) {
      auto &signal = Base::template getSignal<Event>();
      signal(std::forward<Event>(event));
    }
  };

}  // namespace libp2p::event

#endif  // KAGOME_EMITTER_HPP
