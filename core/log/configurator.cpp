/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "log/configurator.hpp"

namespace kagome::log {

  namespace {
    std::string embedded_config(R"(
# ----------------
sinks:
  - name: console
    type: console
    thread: id
    color: true
groups:
  - name: main
    sink: console
    level: info
    children:
      - name: libp2p
      - name: kagome
        children:
          - name: application
          - name: rpc
            children:
            - name: rpc_transport
            - name: api
              children:
                - name: author_api
          - name: authorship
          - name: blockchain
          - name: authority
          - name: crypto
            children:
              - name: bip39
              - name: crypto_store
              - name: ed25519
          - name: consensus
            children:
              - name: babe
                children:
                  - name: babe_lottery
                  - name: babe_synchronizer
                  - name: block_executor
                  - name: block_validator
              - name: grandpa
                children:
                  - name: voting_round
          - name: runtime
            children:
              - name: wasm
              - name: extentions
              - name: binatien
          - name: network
          - name: changes_trie
          - name: storage
          - name: pubsub
          - name: transactions
          - name: testing
            level: debug
# ----------------
  )");
  }

  Configurator::Configurator(std::shared_ptr<PrevConfigurator> previous)
      : ConfiguratorFromYAML(std::move(previous), embedded_config) {}

  Configurator::Configurator(std::shared_ptr<PrevConfigurator> previous,
                             std::string config)
      : ConfiguratorFromYAML(std::move(previous), std::move(config)) {}

  Configurator::Configurator(std::shared_ptr<PrevConfigurator> previous,
                             boost::filesystem::path path)
      : ConfiguratorFromYAML(std::move(previous),
                             std::filesystem::path(path.string())) {}
}  // namespace kagome::log
