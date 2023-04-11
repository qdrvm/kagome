/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_GRANDPACONTEXT
#define KAGOME_CONSENSUS_GRANDPA_GRANDPACONTEXT

#include "consensus/grandpa/structs.hpp"
#include "network/types/grandpa_message.hpp"

#include <set>

#include <libp2p/peer/peer_id.hpp>

namespace kagome::consensus::grandpa {

  class GrandpaContext final {
   public:
    // RAII holder of context on the stack
    class Guard {
      // Avoid creation of guard in the heap
      void *operator new(size_t) = delete;
      void *operator new(size_t, void *) = delete;
      void *operator new[](size_t) = delete;
      void *operator new[](size_t, void *) = delete;

     public:
      Guard() {
        // Create context in ctor
        create();
      }

      ~Guard() {
        // Release context in the out of scope
        release();
      }
    };

    // Payload
    std::optional<const libp2p::peer::PeerId> peer_id{};
    std::optional<const network::VoteMessage> vote{};
    std::optional<const network::CatchUpResponse> catch_up_response{};
    std::optional<const network::FullCommitMessage> commit{};
    std::set<primitives::BlockInfo, std::greater<primitives::BlockInfo>>
        missing_blocks{};
    size_t checked_signature_counter = 0;
    size_t invalid_signature_counter = 0;
    size_t unknown_voter_counter = 0;

    static void set(std::shared_ptr<GrandpaContext> context) {
      auto &opt = instance();
      if (opt.has_value()) {
        throw std::runtime_error(
            "Attempt to set GrandpaContext in thread, "
            "which has already have it set");
      }
      opt.emplace(std::move(context));
    }

    static std::optional<std::shared_ptr<GrandpaContext>> get() {
      auto opt = instance();
      return opt;
    }

   private:
    static std::optional<std::shared_ptr<GrandpaContext>> &instance() {
      static thread_local std::optional<std::shared_ptr<GrandpaContext>> inst;
      return inst;
    }

    static void create() {
      auto &opt = instance();
      if (not opt.has_value()) {
        opt.emplace(std::make_shared<GrandpaContext>());
      }
    }

    static std::shared_ptr<GrandpaContext> release() {
      auto &opt = instance();
      auto ptr = std::move(opt.value());
      opt.reset();
      return ptr;
    }
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_GRANDPACONTEXT
