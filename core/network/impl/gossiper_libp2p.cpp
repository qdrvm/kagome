/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/gossiper_libp2p.hpp"

#include <algorithm>
#include <unordered_set>

#include "network/gossiper_client.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, GossiperLibp2p::Error, e) {
  using E = kagome::network::GossiperLibp2p::Error;
  switch (e) {
    case E::SUCCESS:
      return "success";
    case E::INVALID_CONFIG:
      return "invalid config was provided to GossiperLibp2p";
  }
  return "unknown error";
}

namespace kagome::network {
  GossiperLibp2p::GossiperLibp2p(
      std::shared_ptr<NetworkState> network_state,
      GossiperConfig config,
      std::shared_ptr<crypto::RandomGenerator> random_generator,
      common::Logger log)
      : network_state_{std::move(network_state)},
        config_{config},
        random_generator_{std::move(random_generator)},
        log_{std::move(log)} {}

  void GossiperLibp2p::blockAnnounce(
      BlockAnnounce block_announce,
      std::function<void(const outcome::result<void> &)> cb) const {
    // choose an appropriate set of clients depending on the strategy
    std::vector<std::shared_ptr<GossiperClient>> clients_to_send;
    switch (config_.strategy) {
      case GossiperConfig::Strategy::RANDOM_N:
        clients_to_send = strategyRandomN();
        break;
      default:
        log_->error("Some error in strategy field of the config");
        return cb(Error::INVALID_CONFIG);
    }

    // send message to the chosen clients
    for (const auto &client : clients_to_send) {
      client->blockAnnounce(
          block_announce, [self{shared_from_this()}, client](auto &&write_res) {
            if (!write_res) {
              self->log_->error("cannot write block announce: {}",
                                write_res.error().message());
            }
          });
    }
  }

  std::vector<std::shared_ptr<GossiperClient>> GossiperLibp2p::strategyRandomN()
      const {
    const auto kMaximumNumberOfClientsToChoose =
        std::min(config_.random_n, network_state_->gossiper_clients.size());
    std::unordered_set<size_t> chosen_clients_numbers{};

    // generate a random sequence of bytes...
    auto random_bytes = random_generator_->randomBytes(config_.random_n);
    for (size_t i = 0; i < config_.random_n; ++i) {
      // ...make sure they lie in the bounds of the requested clients numbers or
      // the number of known clients...
      auto client_number = random_bytes[i] % kMaximumNumberOfClientsToChoose;
      // ...and that they do not repeat
      while (chosen_clients_numbers.find(client_number)
             != chosen_clients_numbers.end()) {
        ++client_number;
        if (client_number >= config_.random_n) {
          client_number = 0;
        }
      }
      chosen_clients_numbers.insert(client_number);
    }

    std::vector<std::shared_ptr<GossiperClient>> result{};
    result.reserve(kMaximumNumberOfClientsToChoose);
    std::transform(chosen_clients_numbers.begin(),
                   chosen_clients_numbers.end(),
                   result.begin(),
                   [this](auto number) {
                     return network_state_->gossiper_clients[number];
                   });
    return result;
  }
}  // namespace kagome::network
