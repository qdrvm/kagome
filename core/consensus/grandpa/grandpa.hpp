/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_HPP

#include <boost/sml.hpp>

namespace kagome::consensus::grandpa {

  namespace states {
    struct Start;
    struct Proposed;
    struct Prevoted;
    struct Precommitted;
  }  // namespace states

  namespace events {
    struct Propose;
    struct Prevote;
    struct Precommit;
    struct Commit;
  }  // namespace events

  struct GrandpaSM final {
    template <typename Message>
    auto broadcast(const Message &msg) {}

    auto operator()() {
      using namespace boost::sml;  // NOLINT
      using namespace states;
      using namespace events;
      // clang-format off
      return make_transition_table(
         *state<Start>        + event<Propose>   = state<Proposed>,
         *state<Start>        + event<Prevote>   = state<Prevoted>,
          state<Proposed>     + event<Prevote>   = state<Prevoted>,
          state<Prevoted>     + event<Precommit> = state<Precommitted>,
          state<Precommitted> + event<Commit>    = X
      );
      // clang-format on
    }
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_GRANDPA_HPP
